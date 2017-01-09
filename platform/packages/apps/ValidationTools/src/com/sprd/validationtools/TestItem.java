
package com.sprd.validationtools;

public class TestItem {
    private int indexInAll;
    private int result;

    public TestItem(int indexInAll) {
        this.indexInAll = indexInAll;
    }

    /* SPRD: Modify for bug464743,Some phones don't support Led lights test.{@ */
    public Class getTestClass() {
        return Const.IS_SUPPORT_LED_TEST ? Const.ALL_TEST_ITEM[indexInAll]
                : Const.ALL_TEST_ITEM2[indexInAll];
    }

    public int getTestTitle() {
        return Const.IS_SUPPORT_LED_TEST ? Const.ALL_TEST_ITEM_STRID[indexInAll]
                : Const.ALL_TEST_ITEM_STRID2[indexInAll];
    }

    public String getTestname() {
        return Const.IS_SUPPORT_LED_TEST ? Const.ALL_TEST_ITEM_NAME[indexInAll]
                : Const.ALL_TEST_ITEM_NAME2[indexInAll];
    }
    /* @} */

    public void setResult(int result) {
        this.result = result;
    }

    public int getResult() {
        return result;
    }
}
