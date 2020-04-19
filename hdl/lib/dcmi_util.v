`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    08:21:34 04/15/2020 
// Design Name: 
// Module Name:    DCMITest 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////


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
    input START,
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
    if (START) begin
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
    input START,

    // DCMI master interface
    output [7:0] DATA,
    output       DSYNC,
    input        CLKEN, // One CLK period strobe for updating DCMI output 

    // Global clock
    input CLK
    );

localparam MAX_LEN = 1 << LEN_BITS;
reg [7:0] data_ram[0:MAX_LEN-1];
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
        data_ram[data_addr] <= DI;
        data_addr <= data_addr + 1;
    end
    if (CLKEN)
        tx_trig <= 0;
    if (START) begin
        tx_trig <= 1;
        data_len <= data_addr;
        data_addr <= 0;
    end
    if (CLKEN) begin
        if (tx_trig) begin
            tx_active <= 1;
            data_out <= data_ram[data_addr];
            data_addr <= data_addr + 1;
        end
        if (tx_active) begin
            data_out <= data_ram[data_addr];
            if (data_addr == data_len)
                tx_active <= 0;
            data_addr <= data_addr + 1;
        end
    end
end

endmodule
