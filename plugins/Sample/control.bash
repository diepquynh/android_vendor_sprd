
function disable_addon_module()
{
    local param="$@"
    local dir="`dirname $param`"
    sed -e 's?^include $(BUILD_ADDON_PACKAGE)?#include $(BUILD_ADDON_PACKAGE)?g' $dir/Android.mk > $dir/Android.mk.bak
    mv $dir/Android.mk.bak $dir/Android.mk
}

function disable_all_addon_modules()
{
    # find all Android.mk
    find -name Android.mk > .tmplist
    # convert to command
    sed -e "s?^./?disable_addon_module ./?g" .tmplist > .tmpcommand.bash
    source .tmpcommand.bash
    rm .tmplist
    rm .tmpcommand.bash
}

function enable_addon_module()
{
    local param="$@"
    local dir="`dirname $param`"
    sed -e 's?#include $(BUILD_ADDON_PACKAGE)?include $(BUILD_ADDON_PACKAGE)?g' $dir/Android.mk > $dir/Android.mk.bak
    mv $dir/Android.mk.bak $dir/Android.mk
}

function enable_all_addon_modules()
{
    # find all Android.mk
    find -name Android.mk > .tmplist
    # convert to command
    sed -e "s?^./?enable_addon_module ./?g" .tmplist > .tmpcommand.bash
    source .tmpcommand.bash
    rm .tmplist
    rm .tmpcommand.bash
}

function convert_normal_to_addon()
{
    local dir=`dirname $@`
    sed -e 's?$(BUILD_PACKAGE)?$(BUILD_ADDON_PACKAGE)?g' $dir/Android.mk > $dir/Android.mk.bak
    mv $dir/Android.mk.bak $dir/Android.mk
}
