/*
 *  io_sched.h - I/O scheduler.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-12 Zhang Ziyi
 *  Initial version.
 */

#include "cp_log_cmn.h"
#include "io_sched.h"
#include "log_file.h"

IoScheduler::IoScheduler()
    :m_channel{nullptr},
     m_file{nullptr},
     m_file_wr_client{nullptr},
     m_file_wr_cb{nullptr},
     m_block_size{65536},
     m_max_blocks{8},
     m_commit_threshold{65536},
     m_writing_len{0},
     m_cur_req{io_result_callback, this,
               IoChannel::IRT_WRITE, nullptr,
               nullptr, 0, 0},
     m_buf_avail_cb{nullptr},
     m_buf_client{nullptr},
     m_report_buf_avail{false} {}

IoScheduler::~IoScheduler() {
  if (m_data_written.size()) {
    m_channel->wait_io();
    process_io_result();
  }

  free_buffers(m_buffers);
  clear_ptr_container(m_data);
  clear_ptr_container(m_data_written);
}

void IoScheduler::free_buffers(std::stack<DataBuffer*>& buffers) {
  while (!buffers.empty()) {
    DataBuffer* buf = buffers.top();
    buffers.pop();
    delete buf;
  }
}

void IoScheduler::set_file_written_callback(void* client,
                                            file_written_callback_t cb) {
  m_file_wr_client = client;
  m_file_wr_cb = cb;
}

void IoScheduler::set_buffer_size(size_t max_buf, size_t max_num) {
  m_block_size = max_buf;
  m_max_blocks = max_num;
}

void IoScheduler::set_commit_threshold(size_t size) {
  m_commit_threshold = size;
}

int IoScheduler::init_buffer() {
  DataBuffer* buf;

  for (size_t i = 0; i < m_max_blocks; ++i) {
    buf = alloc_data_buf(m_block_size);
    m_buffers.push(buf);
  }

  return 0;
}

void IoScheduler::set_buf_avail_callback(void* client, buffer_avail_callback_t cb) {
  m_buf_avail_cb = cb;
  m_buf_client = client;
}

void IoScheduler::bind(IoChannel* chan) {
  m_channel = chan;
  chan->set_block_num_hint(m_max_blocks);
}

int IoScheduler::open(LogFile* file) {
  m_file = file;
  return 0;
}

int IoScheduler::close() {
  if (m_data_written.size()) {
    m_channel->wait_io();
    process_io_result();
    process_write_offset();
  }
  m_file = nullptr;
  return 0;
}

int IoScheduler::enqueue(DataBuffer* buf) {
  m_data.push_back(buf);
  if (!m_data_written.size()) {
    commit_data();
  }

  return 0;
}

void IoScheduler::process_write_offset() {
  // Process writing at offset first
  auto it = m_data.begin();

  while (it != m_data.end()) {
    DataBuffer* buf = *it;

    if (buf->dst_offset >= 0) {
      it = m_data.erase(it);
      m_file->write_data_to_offset(buf->buffer + buf->data_start,
                                   buf->data_len,
                                   buf->dst_offset);
      free_buffer(buf);
    } else {
      ++it;
    }
  }
}

void IoScheduler::commit_data() {
  process_write_offset();

  // Process queued data
  size_t data_size = 0;
  unsigned i;

  for (i = 0; i < m_data.size(); ++i) {
    data_size += m_data[i]->data_len;
  }
  if (data_size < m_commit_threshold &&
      m_data.size() < m_max_blocks / 2) {
    // Data not enough
    return;
  }

  for (i = 0; i < m_data.size(); ++i) {
    m_data_written.push_back(m_data[i]);
  }
  m_data.clear();
  m_writing_len = data_size;

  m_cur_req.type = IoChannel::IRT_WRITE;
  m_cur_req.file = m_file;
  m_cur_req.data_list = &m_data_written;
  m_channel->request(&m_cur_req);
}

void IoScheduler::commit_all_data() {
  process_write_offset();

  m_writing_len = 0;
  for (auto it = m_data.begin(); it != m_data.end(); ++it) {
    m_data_written.push_back(*it);
    m_writing_len += (*it)->data_len;
  }
  m_data.clear();
  m_cur_req.type = IoChannel::IRT_WRITE;
  m_cur_req.file = m_file;
  m_cur_req.data_list = &m_data_written;
  m_channel->request(&m_cur_req);
}

void IoScheduler::discard_queue() {
  for (auto it = m_data.begin(); it != m_data.end(); ++it) {
    free_buffer(*it);
  }
  m_data.clear();
}

int IoScheduler::flush() {
  int ret = 0;

  if (m_file) {
    if (m_data_written.size()) {
      m_channel->wait_io();
      process_io_result();
    }

    if (m_data.size()) {
      commit_all_data();
      m_channel->wait_io();
      process_io_result();
    }
  }

  return ret;
}

DataBuffer* IoScheduler::get_free_buffer() {
  DataBuffer* buf{nullptr};

  if (m_buffers.size()) {
    buf = m_buffers.top();
    m_buffers.pop();
  }

  if (!buf) {
    m_report_buf_avail = true;
    //{Debug
    info_log("no free buffer");
  }

  return buf;
}

void IoScheduler::free_buffer(DataBuffer* buf) {
  buf->data_start = buf->data_len = 0;
  buf->dst_offset = -1;
  m_buffers.push(buf);
}

void IoScheduler::process_io_result() {
  if (m_cur_req.written) {
    size_t len = m_cur_req.written;

    m_file->add_size(len);

    auto it = m_data_written.begin();
    unsigned i;

    for (i = 0; i < m_data_written.size(); ++i, ++it) {
      DataBuffer* buf = m_data_written[i];

      if (len >= buf->data_len) {
        len -= buf->data_len;

        free_buffer(buf);
      } else {  // The block not finished
        buf->data_start += len;
        buf->data_len -= len;
        len = 0;
        break;
      }
    }
    if (it != m_data_written.end()) {  // Unfinished data
      m_data.insert(m_data.begin(), it, m_data_written.end());
    }
  } else {  // No data written
    err_log("write data error %d, %u bytes lost",
            m_cur_req.err_code,
            static_cast<unsigned>(m_writing_len));

    // Return the buffers to the idle list
    for (unsigned i = 0; i < m_data_written.size(); ++i) {
      free_buffer(m_data_written[i]);
    }
  }
  m_data_written.clear();
  m_writing_len = 0;
}

void IoScheduler::io_result_callback(void* client,
                                     IoChannel::IoRequest* req) {
  IoScheduler* sched = static_cast<IoScheduler*>(client);

  sched->process_io_result();

  // Notify the client of the file written
  if (sched->m_file_wr_cb) {
    sched->m_file_wr_cb(sched->m_file_wr_client, req->err_code);
  }

  // More data to write?
  if (sched->m_file) {
    sched->commit_data();
  }

  if (sched->m_report_buf_avail && sched->m_buffers.size() &&
      sched->m_buf_avail_cb) {
    sched->m_report_buf_avail = false;
    sched->m_buf_avail_cb(sched->m_buf_client);
  }
}
