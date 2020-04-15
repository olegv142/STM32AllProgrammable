`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    08:34:36 04/13/2020 
// Design Name: 
// Module Name:    echo 
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

module echo(
    // SPI slave interface
    input SCLK,
    input MOSI,
    output MISO,
    input nCS,
    // DCMI master interface
    output [7:0] DATA,
    output       DSYNC,
    output       DCLK,
    // Global clock
    input Clk,
    // On board LEDs
    output [1:0] Led
    );

wire [7:0]  rxd;
wire [7:0]  txd;
wire [7:0]  addr;
wire        sel;
wire        txe;
wire        rxe;

SPIGate gate(
	.SCLK(SCLK),
	.MOSI(MOSI),
	.MISO(MISO),
	.nCS(nCS),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.TXE(txe),
	.RXE(rxe),
	.CLK(Clk)
);

wire [7:0] p0data;
assign Led = ~p0data[1:0];

IOPort8 port0 (
	.ADDRESS(8'h0),
	.DI(8'hde),
	.DO(p0data),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.TXE(txe),
	.RXE(rxe),
	.CLK(Clk)
);

wire [7:0] p1data;

IOPort8 port1 (
	.ADDRESS(8'h1),
	.DI(p1data),
	.DO(p1data),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.TXE(txe),
	.RXE(rxe),
	.CLK(Clk)
);

wire [15:0] p2data;

IOPort16 port2 (
	.ADDRESS(8'h2),
	.DI(p2data),
	.DO(p2data),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.TXE(txe),
	.RXE(rxe),
	.CLK(Clk)
);

/*
wire [7:0] p3data;

IOPort8 #(.ONE_SHOT(1)) port3 (
	.ADDRESS(8'h3),
	.DI(8'hdc),
	.DO(p3data),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.TXE(txe),
	.RXE(rxe),
	.CLK(Clk)
);

wire dcmi_start = p3data[0];

DCMITester dcmi_test (
    .START(dcmi_start),
    .DATA(DATA),
    .DSYNC(DSYNC),
    .DCLK(DCLK),
    .Clk(Clk)
);
*/

wire [7:0] p4data;
wire p4strobe;
wire p4start;

IOPort8 port4 (
	.ADDRESS(8'h4),
	.DI(8'hdc),
	.DO(p4data),
    .STRB(p4strobe),
    .STRT(p4start),
    .DONE(p4done),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.TXE(txe),
	.RXE(rxe),
	.CLK(Clk)
);

DCMITransmitter dcmi_tx (
    .DI(p4data),
    .WR(p4strobe),
    .RST(p4start),
    .START(p4done),
    .DATA(DATA),
    .DSYNC(DSYNC),
    .DCLK(DCLK),
    .Clk(Clk)
    );

endmodule
