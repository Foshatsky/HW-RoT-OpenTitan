// Experiment 3.07: Formal Property Verification (FPV)
// 3-state FSM with formal assertions
module fsm_props(input wire clk, input wire rst, input wire start,
                 output reg [1:0] state, output reg done);

  parameter IDLE   = 2'b00;
  parameter ACTIVE = 2'b01;
  parameter ERROR  = 2'b10;

  reg [1:0] next_state;
  reg was_error;

  // Combinational next state
  always @(*) begin
    case (state)
      IDLE:   next_state = start ? ACTIVE : IDLE;
      ACTIVE: next_state = ERROR;
      ERROR:  next_state = IDLE;
      default: next_state = IDLE;
    endcase
  end

  // Sequential logic and assertions
  always @(posedge clk) begin
    if (rst) begin
      state <= IDLE;
      was_error <= 0;
    end else begin
      state <= next_state;
      was_error <= (state == ERROR);

      // Assert 1: state must never be invalid
      assert(state != 2'b11);

      // Assert 2: if previous cycle was ERROR, current state is IDLE
      if (was_error)
        assert(state == IDLE);

      // Assert 3: combinational next_state is always valid
      assert(next_state == IDLE || next_state == ACTIVE || next_state == ERROR);
    end
  end

  assign done = (state == IDLE);

endmodule
