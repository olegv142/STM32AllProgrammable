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

module DCMITester (
    // Frame start trigger
    input START,
    // DCMI master interface
    output [7:0] DATA,
    output       DSYNC,
    output       DCLK,
    // Global clock
    input Clk
    );

parameter DIV_BITS = 1;

reg [DIV_BITS-1:0] clk_div;
wire clk_en = &clk_div;

always @(posedge Clk)
begin
    clk_div <= clk_div + 1;
end

assign DCLK = clk_div[DIV_BITS-1];

parameter LEN_BITS = 1;

reg tx_trig;
reg tx_active = 0;
reg [LEN_BITS-1:0] cnt = 0;
reg [7:0] data_out;

assign DATA = data_out;
assign DSYNC = tx_active;

always @(posedge Clk)
begin
    if (clk_en)
        tx_trig <= 0;
    if (START)
        tx_trig <= 1;
    if (clk_en) begin
        if (tx_trig) begin
            tx_active <= 1;
            data_out <= 1;
        end
        if (tx_active) begin
            cnt <= cnt + 1;
            data_out <= data_out + 1;
        end
        if (&cnt)
            tx_active <= 0;
    end
end

endmodule

module DCMITransmitter (
    // Data input API
    input [7:0] DI,  // Data input
    input       WR,  // Write enable
    input       RST, // Write pointer reset

    // Frame start trigger
    input START,

    // DCMI master interface
    output [7:0] DATA,
    output       DSYNC,
    output       DCLK,

    // Global clock
    input Clk
    );

parameter DIV_BITS = 1;

reg [DIV_BITS-1:0] clk_div;
wire clk_en = &clk_div;

always @(posedge Clk)
begin
    clk_div <= clk_div + 1;
end

assign DCLK = clk_div[DIV_BITS-1];

parameter LEN_BITS = 10;
parameter MAX_LEN = 1 << LEN_BITS;
reg [7:0] data_ram[0:MAX_LEN-1];
reg [LEN_BITS-1:0] data_addr;

reg tx_trig;
reg tx_active = 0;
reg [7:0] data_out;
reg [LEN_BITS-1:0] data_len;

assign DATA = data_out;
assign DSYNC = tx_active;

always @(posedge Clk)
begin
    if (RST)
        data_addr <= 0;
    if (WR) begin
        data_ram[data_addr] <= DI;
        data_addr <= data_addr + 1;
    end
    if (clk_en)
        tx_trig <= 0;
    if (START) begin
        tx_trig <= 1;
        data_len <= data_addr;
        data_addr <= 0;
    end
    if (clk_en) begin
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
