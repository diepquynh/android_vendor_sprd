
package com.sprd.firewall.model;

public class BlackCallEntity extends BlackEntity {

    private long time;

    private Integer type;

    public long getTime() {
        return time;
    }

    public void setTime(long time) {
        this.time = time;
    }

    public Integer getType() {
        return type;
    }

    public void setType(Integer type) {
        this.type = type;
    }
}
