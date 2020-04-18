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

// The gate instance provide interface between external SPI bus
// and internal parallel bus where IO ports are attached

SPIGate spi_gate(
	.SCLK(SCLK),
	.MOSI(MOSI),
	.MISO(MISO),
	.nCS(nCS),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.RXE(rxe),
	.CLK(Clk)
);

wire [7:0] mdata;
wire dclken;
wire [1:0] dreq;
wire [1:0] dack;

DCMIGate #(.M(2)) dcmi_gate(
    .DATA(DATA),
    .DSYNC(DSYNC),
    .DCLK(DCLK),
    .MDATA(mdata),
    .DCLKEN(dclken),
    .DREQ(dreq),
    .DACK(dack),
    .CLK(Clk)
);

//
// Port #0 controls on-board LEDs
//
wire [7:0] p0data;
assign Led = ~p0data[1:0];

IOPort8 #(.ADDRESS(0)) port0 (
	.DI(8'hde),
	.DO(p0data),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.RXE(rxe),
	.CLK(Clk)
);

//
// Port #1 outputs the same data as was written to it
//

wire [7:0] p1out;
reg  [7:0] p1data = 0;
wire p1strobe;

IOPort8 #(.ADDRESS(1), .ONE_SHOT(1)) port1 (
	.DI(p1data),
	.DO(p1out),
    .STRB(p1strobe),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.RXE(rxe),
	.CLK(Clk)
);

// We don't really need to latch data in order to echo it back
// This code here just to verify that we can do it
always @(posedge Clk)
begin
    if (p1strobe)
        p1data <= p1out;
end

//
// Port #2 is 16 bit wide. It outputs the same data as was written to it.
// The only difference with 8 bit port is that the latter may echo stream of data
// while the 16 bit version is not suitable for streaming.
//

wire [15:0] p2data;

IOPort16 #(.ADDRESS(2)) port2 (
	.DI(p2data),
	.DO(p2data),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.RXE(rxe),
	.CLK(Clk)
);

// Port #3 triggers DCMI test frame transmission

wire [7:0] p3data;

IOPort8 #(.ADDRESS(3), .ONE_SHOT(1)) port3 (
	.DI(8'hdc),
	.DO(p3data),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.RXE(rxe),
	.CLK(Clk)
);

DCMITester dcmi_test (
    .START(p3data[0]),
    .MDATA(mdata),
    .DCLKEN(dclken),
    .DREQ(dreq[0]),
    .DACK(dack[0]),
    .CLK(Clk)
);

//
// Port #4 serves for DCMI bus testing
// The data written to it is buffered and then
// transmitted to DCMI bus once the port is deselected
//

wire [7:0] p4data;
wire p4strobe;
wire p4start;

IOPort8 #(.ADDRESS(4)) port4 (
	.DI(8'hdc),
	.DO(p4data),
    .STRB(p4strobe),
    .STRT(p4start),
    .DONE(p4done),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.RXE(rxe),
	.CLK(Clk)
);

DCMITxBuff dcmi_tx (
    .DI(p4data),
    .WR(p4strobe),
    .RST(p4start),
    .START(p4done),
    .MDATA(mdata),
    .DCLKEN(dclken),
    .DREQ(dreq[1]),
    .DACK(dack[1]),
	.CLK(Clk)
    );

//
// Port #5 transmit the sequence of sequential numbers.
// This is the example of the streaming data to MCU using IOPort8
// If you need to stream data in the opposite direction via SPI bus
// see DCMITransmitter implementation
//

reg [7:0] p5data;
wire p5strobe;
wire p5start;

IOPort8 #(.ADDRESS(5)) port5 (
	.DI(p5data),
    .STRB(p5strobe),
    .STRT(p5start),
	.RXD(rxd),
	.TXD(txd),
	.ADDR(addr),
	.SEL(sel),
	.RXE(rxe),
	.CLK(Clk)
);

always @(posedge Clk)
begin
    if (p5start)
        p5data <= 'b0;
    if (p5strobe)
        p5data <= p5data + 1;
end

endmodule
