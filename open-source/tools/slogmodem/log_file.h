/*
 *  log_file.h - log file class.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-5-14 Zhang Ziyi
 *  Initial version.
 */
#ifndef _LOG_FILE_H_
#define _LOG_FILE_H_

#include "cp_log_cmn.h"

class CpDirectory;

class LogFile {
 public:
  enum LogType {
    LT_UNKNOWN,
    LT_LOG,
    LT_SIPC,
    LT_DUMP,
    LT_MINI_DUMP,
    LT_VERSION,
    LT_RINGBUF,
    LT_SLEEPLOG,
    LT_WCDMA_IQ
  };

  struct FileTime {
    int year;
    int month;
    int mday;
    int hour;
    int min;
    int sec;

    bool operator<(const FileTime& t) const;
  };

  // Constructor for general file
  LogFile(const LogString& base_name, CpDirectory* dir,
          LogType type = LT_UNKNOWN, size_t sz = 0);
  // Constructor for LT_LOG file
  LogFile(const LogString& base_name, CpDirectory* dir,
          const struct tm& file_time);
  ~LogFile();

  const LogString& base_name() const { return m_base_name; }

  CpDirectory* dir() { return m_dir; }

  /*  get_size - get the file size and save the size in member.
   *
   *  This function will only calculate the size of data that is
   *  written into the file. It won't count the data that is in
   *  user space buffers, such as stdio buffers.
   *
   *  Return 0 on success, -1 on failure.
   */
  int get_size();

  /*  get_type - determine the file type and save the type.
   *
   *  This function tries to determine the file type according to
   *  its name. If the file is of type LT_LOG, its time will also
   *  be extracted from the name and saved into m_time.
   *
   *  Return the file type.
   */
  LogType get_type();

  LogType type() const { return m_type; }

  size_t size() const { return m_size; }

  /*  create - create the file.
   *
   *  Return 0 on success, -1 on failure.
   */
  int create();

  /*  close - close the file.
   *
   *  Return 0 on success, -1 on failure.
   */
  int close();

  /*  write - write file and update the file size.
   *  @data: the pointer to the data to be written.
   *  @len: the length of the data in byte.
   *
   *  Return the number of bytes written on success, -1 on general error,
   *  -2 if the disk is full.
   */
  ssize_t write(const void* data, size_t len);
  /*  write_raw - write file directly to disk without update the size
   *              member.
   *  @data: the pointer to the data to be written.
   *  @len: the length of the data in byte.
   *
   *  Return the number of bytes written on success, -1 on general error,
   *  -2 if the disk is full.
   */
  ssize_t write_raw(const void* data, size_t len);

  /*  count_size - update the file size.
   *  @size: size of the file.
   *
   *  This function is used after write_raw.
   */
  void count_size(size_t size);

  /*  discard - discard all file data.
   *
   */
  int discard();

  int flush();

  bool exists() const;
  /*  remove - remove the file from the disk. Don't decrease any size.
   *  @par_dir: the parent directory.
   *
   *  Return 0 on success, -1 otherwise.
   */
  int remove(const LogString& par_dir);

  /*  operator < - operator < for m_time.
   */
  bool operator<(const LogFile& f) const { return m_time < f.m_time; }

  int rotate(const LogString& par_path);

  int change_name_num(unsigned num);

  int copy(const char* src_file);

  static int parse_log_file_num(const char* name, size_t len, unsigned& num,
                                size_t& nlen);

 private:
#define FILE_IO_BUF_SIZE (1024 * 64)

  // The directory where the file locates
  CpDirectory* m_dir;
  // File base name
  LogString m_base_name;
  // Log file type
  LogType m_type;
  // File time in file name
  FileTime m_time;
  // File size in byte
  size_t m_size;
  // File descriptor
  int m_fd;
  // Data buffer
  uint8_t* m_buffer;
  size_t m_buf_len;
  size_t m_data_len;

  /*  write_data - write data into the file.
   *
   *  Return the number of bytes written into the file. -1 on failure.
   */
  ssize_t write_data(const void* data, size_t len);

  /*  parse_file_time - parse the time in the file name.
   *  @name: the time string starting at year part.
   *  @len: the length of name in byte.
   *  @ft: the file time.
   *
   *  Return 0 on success, -1 on error.
   */
  static int parse_time_string(const char* name, size_t len, FileTime& ft);

  /*  parse_dump_file_time - parse the time in the dump file name.
   *  @name: the file name segment of interest.
   *  @len: the length of name in byte.
   *  @ft: the file time.
   *
   *  Return 0 on success, -1 on error.
   */
  static int parse_dump_file_time(const char* name, size_t len, FileTime& ft);

  static const char* get_4digits(const char* s, size_t len);
  static int extract_file_time(const char* name, size_t len, FileTime& ft);
};

#endif  // !_LOG_FILE_H_
