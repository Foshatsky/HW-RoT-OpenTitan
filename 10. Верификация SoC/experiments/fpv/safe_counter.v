// Experiment 3.07: Formal Property Verification (FPV)
// Safe counter that counts 0..2 and wraps
module safe_counter(input wire clk, input wire rst, input wire en,
                    output reg [1:0] count);

  wire [1:0] next_count;

  // Combinational next value
  assign next_count = (count == 2'd2) ? 2'd0 : count + 2'd1;

  // Sequential logic and assertions
  always @(posedge clk) begin
    if (rst) begin
      count <= 0;
    end else if (en) begin
      count <= next_count;
    end

    if (!rst && en) begin
      // Assert 1: count never exceeds 2
      assert(next_count <= 2'd2);

      // Assert 2: if count is 2, next must be 0 (wrap)
      if (count == 2'd2)
        assert(next_count == 2'd0);
      else
        // Assert 3: otherwise count increments by 1
        assert(next_count == count + 2'd1);
    end
  end

endmodule
