/*
** Copyright 2016, Spreadtrum
*/
#ifdef EX_INSTALLD
//SPRD: add sprd installd  @{
static int do_backup_app(char **arg, char reply[REPLY_MAX] __unused)
{
    return backup_app(arg[0], arg[1], atoi(arg[2]), atoi(arg[3]));
}

static int do_restore_app(char **arg, char reply[REPLY_MAX] __unused)
{
    return restore_app(arg[0], arg[1], atoi(arg[2]), atoi(arg[3]));
}
// @}
#endif
