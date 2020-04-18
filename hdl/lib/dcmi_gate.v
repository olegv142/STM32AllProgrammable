//
// DCMI gateway plays role of arbiter for multiple transmitters
//

module DCMIGate
    # (
        parameter M = 1, // Maximum number of multiplexed transmitters
        parameter DIV_BITS = 1
    )
    (
    // DCMI host interface
    output [7:0]   DATA,
    output         DSYNC,
    output         DCLK,
    // Internal interface for multiplexed transmitters
    input [7:0]    MDATA,
    output         DCLKEN,
    // Request / acknowledge signals for multiplexed transmitters
    input  [M-1:0] DREQ,
    output [M-1:0] DACK,
    // Global clock
    input CLK
    );

reg [DIV_BITS-1:0] clk_div;

always @(posedge CLK)
begin
    clk_div <= clk_div + 1;
end

assign DCLK  = clk_div[DIV_BITS-1];
assign DATA  = MDATA;
assign DCLKEN = &clk_div;

reg [M-1:0] ack = 0;
assign DACK = ack & DREQ;
assign DSYNC = |DACK;

integer i;

always @(posedge CLK)
begin
    if (DCLKEN) begin
        if (ack)
            ack <= ack & DREQ;
        else
            for (i = 0; i < M; i = i + 1)
                if (DREQ[i])
                    ack <= 1 << i;
    end
end

endmodule
