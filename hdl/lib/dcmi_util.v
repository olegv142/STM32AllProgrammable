`timescale 1ns / 1ps

//
// Make output clock by dividing it to power of 2
//
module DCMIClkGen
    #(
        parameter DIV_BITS = 1
    )
    (
    output DCLK,  // DCMI output clock
    output CLKEN, // One CLK period strobe for updating DCMI output 

    // Global clock
    input CLK
    );

reg [DIV_BITS-1:0] clk_div;
assign CLKEN = &clk_div;
assign DCLK = clk_div[DIV_BITS-1];

always @(posedge CLK)
begin
    clk_div <= clk_div + 1;
end

endmodule

//
// The DCMITester transmits packet filled with incrementing counter bytes starting from zero
//
module DCMITester 
    #(
        parameter LEN_BITS = 2 // Packet length bits
    )
    (
    // Frame start trigger
    input TX_START,
    // DCMI master interface
    output [7:0] DATA,
    output       DSYNC,
    input        CLKEN,
    // Global clock
    input CLK
    );

reg tx_trig;
reg tx_active = 0;
reg [LEN_BITS-1:0] cnt;

assign DATA = tx_active ? cnt : 8'b0;
assign DSYNC = tx_active;

always @(posedge CLK)
begin
    if (CLKEN)
        tx_trig <= 0;
    if (TX_START) begin
        cnt <= 0;
        tx_trig <= 1;
    end
    if (CLKEN) begin
        if (tx_trig)
            tx_active <= 1;
        if (tx_active)
            cnt <= cnt + 1;
        if (&cnt)
            tx_active <= 0;
    end
end

endmodule


//
// The DCMI transmitter with buffer that can be filled with data
// by writing bytes sequentially. Output the same amount of data
// that were written to the buffer.
//

module DCMITxBuffer 
    #(
        parameter LEN_BITS = 10
    )
    (
    // Data input API
    input [7:0] DI,  // Data input
    input       WR,  // Write enable
    input       RST, // Write pointer reset

    // Frame start trigger
    input TX_START,

    // DCMI master interface
    output [7:0] DATA,
    output       DSYNC,
    input        CLKEN, // One CLK period strobe for updating DCMI output 

    // Global clock
    input CLK
    );

localparam BUFF_SZ = 1 << LEN_BITS;
reg [7:0] data_buff[0:BUFF_SZ-1];
reg [LEN_BITS-1:0] data_addr;

reg tx_trig;
reg tx_active = 0;
reg [7:0] data_out;
reg [LEN_BITS-1:0] data_len;

assign DATA = tx_active ? data_out : 8'b0;
assign DSYNC = tx_active;

always @(posedge CLK)
begin
    if (RST)
        data_addr <= 0;
    if (WR) begin
        data_buff[data_addr] <= DI;
        data_addr <= data_addr + 1;
    end
    if (TX_START) begin
        tx_trig <= 1;
        data_len <= data_addr;
        data_addr <= 0;
    end
    if (CLKEN) begin
        if (tx_trig) begin
            tx_trig <= 0;
            tx_active <= 1;
            data_out <= data_buff[data_addr];
            data_addr <= data_addr + 1;
        end
        if (tx_active) begin
            data_out <= data_buff[data_addr];
            if (data_addr == data_len)
                tx_active <= 0;
            data_addr <= data_addr + 1;
        end
    end
end

endmodule

//
// The DCMI transmitter capturing full buffer of bytes on rising edge of the TRIG.
// Can be used as chip scope for in-system debugging.
//

module DCMICaptureBuffer
    #(
        parameter LEN_BITS = 12,
        parameter DELAY = 8
    )
    (
    input [7:0] DI,   // Data input
    input       TRIG, // Capture starts on rising edge

    // DCMI frame transmit start trigger
    input TX_START,

    // DCMI master interface
    output [7:0] DATA,
    output       DSYNC,
    input        CLKEN, // One CLK period strobe for updating DCMI output 

    // Global clock
    input CLK
    );

localparam BUFF_SZ = 1 << LEN_BITS;
reg [7:0] data_buff[0:BUFF_SZ-1];
reg [LEN_BITS-1:0] data_addr = 0;

reg buff_full = 0;
reg start_req = 0;
reg tx_trig = 0;
reg tx_active = 0;
reg [7:0] data_out;

assign DATA = tx_active ? data_out : 8'b0;
assign DSYNC = tx_active;

integer i;
reg [7:0] data_delay[0:DELAY-1];
wire [7:0] data_delayed = data_delay[DELAY-1];

reg last_capture;
wire capturing = !buff_full && (data_addr != 0);
wire ready_to_capture = ~buff_full & ~capturing;

always @(posedge CLK)
begin
    data_delay[0] <= DI;
    for (i = 1; i < DELAY; i = i + 1)
        data_delay[i] <= data_delay[i-1];
end

always @(posedge CLK)
begin
    last_capture <= TRIG;
end

wire capture_trigger = TRIG & ~last_capture;

always @(posedge CLK)
begin
    if (ready_to_capture && capture_trigger) begin
        data_buff[data_addr] <= data_delayed;
        data_addr <= data_addr + 1;
    end
    if (capturing) begin
        data_buff[data_addr] <= data_delayed;
        data_addr <= data_addr + 1;
        if (&data_addr)
            buff_full <= 1;
    end
    if (TX_START)
        start_req <= 1;
    if (CLKEN) begin
        if (start_req && buff_full) begin
            start_req <= 0;
            tx_trig <= 1;
            data_addr <= 0;
        end
        if (tx_trig) begin
            tx_trig <= 0;
            tx_active <= 1;
            data_out <= data_buff[data_addr];
            data_addr <= data_addr + 1;
        end
        if (tx_active) begin
            data_out <= data_buff[data_addr];
            if (data_addr == 0) begin
                tx_active <= 0;
                buff_full <= 0;
            end else
                data_addr <= data_addr + 1;
        end
    end
end

endmodule
