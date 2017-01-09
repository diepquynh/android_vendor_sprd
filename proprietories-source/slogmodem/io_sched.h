/*
 *  io_sched.h - I/O scheduler.
 *
 *  Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 *  History:
 *  2016-7-12 Zhang Ziyi
 *  Initial version.
 */
#ifndef _IO_SCHED_H_
#define _IO_SCHED_H_

#include <stack>
#include <vector>

#include "data_buf.h"
#include "io_chan.h"

class LogFile;

class IoScheduler {
 public:
  typedef void (*buffer_avail_callback_t)(void* client);
  typedef void (*file_written_callback_t)(void* client, int err);

  IoScheduler();
  ~IoScheduler();

  void set_buffer_size(size_t max_buf, size_t max_num);
  void set_commit_threshold(size_t size);
  int init_buffer();

  /*  bind - bind the scheduler to an IoChannel.
   *
   */
  void bind(IoChannel* chan);

  void set_file_written_callback(void* client, file_written_callback_t cb);
  void set_buf_avail_callback(void* client, buffer_avail_callback_t cb);

  /*  open - prepare I/O for the file.
   *
   */
  int open(LogFile* file);
  int close();

  /*  enqueue - put the data block in the queue.
   *
   *  This function shall be called when m_file is valid.
   *
   *  Return 0 on success, -1 otherwise.
   */
  int enqueue(DataBuffer* buf);

  /*  discard_queue - discard data queued for commit.
   *
   */
  void discard_queue();

  /*  flush - flush data.
   */
  int flush();

  DataBuffer* get_free_buffer();
  void free_buffer(DataBuffer* buf);

 private:
  IoChannel* m_channel;
  // Current file
  LogFile* m_file;
  // File write notification
  void* m_file_wr_client;
  file_written_callback_t m_file_wr_cb;
  // Buffer policies
  // One block size
  size_t m_block_size;
  // Max blocks
  size_t m_max_blocks;
  // Write commit water mark
  size_t m_commit_threshold;
  // Idle buffers
  std::stack<DataBuffer*> m_buffers;
  // Data buffers to be commit to I/O thread
  std::deque<DataBuffer*> m_data;
  // Data blocks that are being written
  std::vector<DataBuffer*> m_data_written;
  // Data length that is being written
  size_t m_writing_len;
  IoChannel::IoRequest m_cur_req;
  // Buffer available callback
  buffer_avail_callback_t m_buf_avail_cb;
  void* m_buf_client;
  bool m_report_buf_avail;

  void commit_data();
  /*  commit_all_data - commit all data to IoChannel
   *
   *  This function assumes m_data is not empty.
   */
  void commit_all_data();
  /*  process_write_offset - write all offset based data into file.
   */
  void process_write_offset();
  void process_io_result();

  static void free_buffers(std::stack<DataBuffer*>& buffers);
  static void io_result_callback(void* client, IoChannel::IoRequest* req);
};

#endif  //!_IO_SCHED_H_
