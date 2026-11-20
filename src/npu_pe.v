`timescale 1ns / 1ps

module npu_pe (
    input  wire               clk,
    input  wire               rst_n,     // active-low reset
    input  wire               load_w,    // 1 = shift weights in
    input  wire signed [7:0]  w_in,      // weight from the PE above
    output wire signed [7:0]  w_out,     // weight to the PE below (directly from w_reg)
    input  wire signed [7:0]  a_in,      // activation from the left
    output reg  signed [7:0]  a_out,     // activation to the right
    input  wire signed [31:0] psum_in,   // partial sum from above
    output reg  signed [31:0] psum_out   // partial sum to below
);

    // declare the internal weight register (signed, 8 bit)
    reg signed [7:0] w_reg;

    // connect w_out straight to the weight register
    assign w_out = w_reg;

    always @(posedge clk) begin
        if (!rst_n) begin
            w_reg    <= 0;
            a_out    <= 0;
            psum_out <= 0;
        end else begin
            if (load_w) begin
                w_reg <= w_in;
            end
            a_out <= a_in;
            psum_out <= psum_in + a_in * w_reg;
        end
    end

endmodule