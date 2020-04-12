`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    17:07:13 04/12/2020 
// Design Name: 
// Module Name:    blink 
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
module blink(
    input Clk,
    input nRst,
    output [1:0] Led
    );
parameter N = 25;
reg[N:0] cnt;

always @(posedge Clk)
begin
	if (!nRst)
		cnt <= 0;
	else
		cnt <= cnt + 1;
end

assign Led[0] = cnt[N];
assign Led[1] = ~cnt[N];

endmodule
