
package com.sprd.firewall.model;

public class BlackNumberEntity extends BlackEntity {

    private String notes;

    private String min_match;

    private Integer type;

    public String getNotes() {
        return notes;
    }

    public void setNotes(String notes) {
        this.notes = notes;
    }

    public String getMinmatch() {
        return min_match;
    }

    public void setMinmatch(String number) {
        this.min_match = number;
    }

    public Integer getType() {
        return type;
    }

    public void setType(Integer type) {
        this.type = type;
    }
}
