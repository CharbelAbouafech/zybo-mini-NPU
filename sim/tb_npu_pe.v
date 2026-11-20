`timescale 1ns / 1ps

// tb_npu_pe.v - self-checking testbench for one processing element (npu_pe)
//
// Pattern for every test: set inputs -> one clock edge -> wait 1 ns -> check outputs.
// Expected values are computed by hand in the comments.

module tb_npu_pe;

    // ---- signals to drive the PE (reg) and to watch its outputs (wire) ----
    reg                clk = 0;
    reg                rst_n = 0;
    reg                load_w = 0;
    reg  signed [7:0]  w_in = 0;
    reg  signed [7:0]  a_in = 0;
    reg  signed [31:0] psum_in = 0;
    wire signed [7:0]  w_out;
    wire signed [7:0]  a_out;
    wire signed [31:0] psum_out;

    integer errors = 0;
    integer f;

    // ---- the device under test ----
    npu_pe dut (
        .clk(clk), .rst_n(rst_n), .load_w(load_w),
        .w_in(w_in), .w_out(w_out),
        .a_in(a_in), .a_out(a_out),
        .psum_in(psum_in), .psum_out(psum_out)
    );

    // ---- 100 MHz clock: toggles every 5 ns ----
    always #5 clk = ~clk;

    // ---- helper: compare a value with the expected one ----
    task check(input [255:0] name, input signed [31:0] got, input signed [31:0] expected);
        begin
            if (got === expected)
                $display("PASS  %0s: %0d", name, got);
            else begin
                $display("FAIL  %0s: got %0d, expected %0d", name, got, expected);
                errors = errors + 1;
            end
        end
    endtask

    // ---- helper: load one weight (one clock with load_w = 1) ----
    task load_weight(input signed [7:0] w);
        begin
            load_w = 1; w_in = w;
            @(posedge clk); #1;
            load_w = 0;
        end
    endtask

    // ---- helper: one MAC step (one clock with the given inputs) ----
    task mac_step(input signed [7:0] a, input signed [31:0] psum);
        begin
            a_in = a; psum_in = psum;
            @(posedge clk); #1;
        end
    endtask

    // ---- the tests ----
    initial begin
        // reset for 2 clocks
        rst_n = 0;
        repeat (2) @(posedge clk);
        #1;
        check("reset: psum_out", psum_out, 0);
        check("reset: a_out",    a_out,    0);
        check("reset: w_out",    w_out,    0);
        rst_n = 1;

        // TEST 1: load weight 3
        load_weight(3);
        check("T1 weight loaded", w_out, 3);

        // TEST 2: simple MAC, 10 + 5 * 3 = 25; the activation passes through
        mac_step(5, 10);
        check("T2 psum_out", psum_out, 25);
        check("T2 a_out",    a_out,    5);

        // TEST 3: negative activation, 7 + (-2) * 3 = 1
        mac_step(-2, 7);
        check("T3 psum_out", psum_out, 1);
        check("T3 a_out",    a_out,    -2);

        // TEST 4: weight is held while load_w = 0, even if w_in changes
        w_in = 99;
        mac_step(0, 0);
        check("T4 weight held", w_out, 3);

        // TEST 5: negative weight, 0 + 3 * (-4) = -12
        load_weight(-4);
        mac_step(3, 0);
        check("T5 psum_out", psum_out, -12);

        // TEST 6a: largest product, (-128) * (-128) = 16384
        load_weight(-128);
        mac_step(-128, 0);
        check("T6a psum_out", psum_out, 16384);

        // TEST 6b: most negative product plus a large partial sum,
        //          1000000 + 127 * (-128) = 1000000 - 16256 = 983744
        mac_step(127, 1000000);
        check("T6b psum_out", psum_out, 983744);

        // TEST 7: negative partial sum from above, -500 + (-1) * (-128) = -372
        mac_step(-1, -500);
        check("T7 psum_out", psum_out, -372);

        // TEST 8: reset in the middle clears everything again
        rst_n = 0;
        @(posedge clk); #1;
        check("T8 reset psum_out", psum_out, 0);
        check("T8 reset w_out",    w_out,    0);
        rst_n = 1;

        // ---- summary ----
        if (errors == 0) $display("ALL TESTS PASSED");
        else             $display("%0d TEST(S) FAILED", errors);

        f = $fopen("results_tb_npu_pe.txt", "w");
        if (errors == 0) $fdisplay(f, "PASS");
        else             $fdisplay(f, "FAIL (%0d errors)", errors);
        $fclose(f);

        $finish;
    end

    // ---- watchdog: stop if something hangs ----
    initial begin
        #10000;
        $display("TIMEOUT");
        f = $fopen("results_tb_npu_pe.txt", "w");
        $fdisplay(f, "FAIL (timeout)");
        $fclose(f);
        $finish;
    end

endmodule
