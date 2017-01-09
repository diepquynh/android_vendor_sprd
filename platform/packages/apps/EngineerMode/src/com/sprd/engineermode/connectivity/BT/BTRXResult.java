
package com.sprd.engineermode.connectivity.BT;

public class BTRXResult {

    public String time;
    public String rssi;
    public String per;
    public String ber;

    public BTRXResult(String time, String rssi, String per, String ber, boolean isOdd) {
        super();
        this.time = time;
        this.rssi = rssi;
        this.per = per;
        this.ber = ber;
    }
}
