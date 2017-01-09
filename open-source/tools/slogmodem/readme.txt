Configuration of the CP's top directory:

1. The modem_log directory location can be determined by the parameters that
are passed to slogmodem, with a "-s" option:

 	service slogmodem /system/bin/slogmodem -s

	The modem_log's directory will be like:
		sdcard
		  |-slog
			|-yy-mm-dd-hh-mm-ss
			|-... ...
			|-modem_log
			|-last_log
