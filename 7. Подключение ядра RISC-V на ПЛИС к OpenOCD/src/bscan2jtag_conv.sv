//
//
module bscan2jtag_conv (
  input wire S_BSCAN_bscanid_en,
  input wire S_BSCAN_capture,
  input wire S_BSCAN_drck,
  input wire S_BSCAN_reset,
  input wire S_BSCAN_runtest,
  input wire S_BSCAN_sel,
  input wire S_BSCAN_shift,
  input wire S_BSCAN_tck,
  input wire S_BSCAN_tdi,
  input wire S_BSCAN_tms,
  input wire S_BSCAN_update,
  output wire S_BSCAN_tdo,
  input wire JTAG_TDO,
  output wire JTAG_TDI,
  output wire JTAG_TMS,
  output wire JTAG_TCK,
  output reg jtag_rstn_o
  );

  wire jtag_clk_w = S_BSCAN_drck;
  wire reset_w    = S_BSCAN_reset;
  wire shift_w    = S_BSCAN_shift;
  wire tdi_w      = S_BSCAN_tdi;

  reg jtag_tck_ena;
  reg jtag_tms_int;

  assign JTAG_TCK = S_BSCAN_tck & jtag_tck_ena; 
  assign JTAG_TMS = jtag_tms_int;
  assign JTAG_TDI = S_BSCAN_tdi;

  assign S_BSCAN_tdo = JTAG_TDO;

  typedef enum logic [3:0] {
    IDLE,
    SELECT_IR,
    SELECT_DR,
    SHIFT_IR,
    SHIFT_DR,
    UPDATE_IR,
    UPDATE_DR
  } fsm_e;

  fsm_e fsm;

  reg [3:0] select_cnt;

  always_ff @ (posedge jtag_clk_w) begin
    if ((fsm == SELECT_IR) || (fsm == SELECT_DR)) select_cnt <= select_cnt + 1'b1;
    else select_cnt <= 4'h0;
  end

  reg [6:0] shift_ir_cnt;
  reg [6:0] shift_dr_cnt;

  always_ff @ (posedge jtag_clk_w) begin
    if (fsm == SHIFT_IR) shift_ir_cnt <= shift_ir_cnt + 1'b1;
    else shift_ir_cnt <= 7'h0;
  end

  always_ff @ (posedge jtag_clk_w) begin
    if (fsm == SHIFT_DR) shift_dr_cnt <= shift_dr_cnt + 1'b1;
    else shift_dr_cnt <= 7'h0;
  end

  reg [1:0] update_cnt;

  always_ff @ (posedge jtag_clk_w) begin
    if ((fsm == UPDATE_IR) || (fsm == UPDATE_DR)) update_cnt <= update_cnt + 1'b1;
    else update_cnt <= 2'h0;
  end

  localparam [3:0] IR_TCK_START = 4'h3;
  localparam [3:0] DR_TCK_START = IR_TCK_START+1;

  always_ff @ (negedge jtag_clk_w or posedge reset_w) begin
    if (reset_w) begin
      jtag_tms_int <= 1'b0;
      jtag_rstn_o <= 1'b0;
    end else begin
      jtag_rstn_o <= 1'b1;

      if (fsm == SELECT_IR) begin
        if (select_cnt == IR_TCK_START) jtag_tms_int <= 1'b1;
        else if (select_cnt == IR_TCK_START+2) jtag_tms_int <= 1'b0;
      end else if (fsm == SELECT_DR) begin
        if (select_cnt == DR_TCK_START) jtag_tms_int <= 1'b1;
        else if (select_cnt == DR_TCK_START+1) jtag_tms_int <= 1'b0;
      end else if ((fsm == UPDATE_IR) || (fsm == UPDATE_DR)) begin
        if (update_cnt == 2'h0) jtag_tms_int <= 1'b1;
        else if (update_cnt == 2'h2) jtag_tms_int <= 1'b0;
      end
    end
  end

  always_ff @ (negedge jtag_clk_w or posedge reset_w) begin
    if (reset_w) begin
      jtag_tck_ena <= 1'b0;
    end else begin
      if (fsm == IDLE) begin 
        jtag_tck_ena <= 1'b0;
      end else begin
        jtag_tck_ena <= 1'b1;
      end
    end
  end

  reg [6:0] data_size;

  always_ff @ (posedge jtag_clk_w) begin
    if ((fsm == SELECT_IR) || (fsm == SELECT_DR)) data_size <= {tdi_w,data_size[6:1]};
  end

  wire [6:0] data_size_corr_w = data_size - 2'h2;

  always_ff @ (posedge jtag_clk_w or posedge reset_w) begin
    if (reset_w) begin
      fsm <= IDLE;
    end else begin
      case (fsm)
        IDLE: begin
          if (shift_w) begin
            if (tdi_w) fsm <= SELECT_DR;
            else fsm <= SELECT_IR;
          end else fsm <= IDLE;
        end
        //
        SELECT_DR: begin
          if (~shift_w) fsm <= IDLE;
          else if (select_cnt == 4'h6) fsm <= SHIFT_DR;
        end
        //
        SELECT_IR: begin
          if (~shift_w) fsm <= IDLE;
          else if (select_cnt == 4'h6) fsm <= SHIFT_IR; 
        end
        //
        SHIFT_DR: begin
          if (~shift_w) fsm <= IDLE;
          else if (shift_dr_cnt == data_size_corr_w) fsm <= UPDATE_DR;
        end
        //
        SHIFT_IR: begin
          if (~shift_w) fsm <= IDLE;
          else if (shift_ir_cnt == data_size_corr_w) fsm <= UPDATE_IR; 
        end
        //
        UPDATE_IR: begin
          if (~shift_w || (update_cnt == 2'h3)) fsm <= IDLE;
        end
        //
        UPDATE_DR: begin
          if (~shift_w || (update_cnt == 2'h3)) fsm <= IDLE;
        end
      endcase
    end
  end


endmodule

