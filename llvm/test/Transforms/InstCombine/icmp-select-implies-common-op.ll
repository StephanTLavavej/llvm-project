; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -passes=instcombine < %s | FileCheck %s

define i1 @sgt_3_impliesF_eq_2(i8 %x, i8 %y) {
; CHECK-LABEL: @sgt_3_impliesF_eq_2(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], 4
; CHECK-NEXT:    [[CMP21:%.*]] = icmp eq i8 [[Y:%.*]], [[X]]
; CHECK-NEXT:    [[CMP2:%.*]] = select i1 [[CMP]], i1 [[CMP21]], i1 false
; CHECK-NEXT:    ret i1 [[CMP2]]
;
  %cmp = icmp sgt i8 %x, 3
  %sel = select i1 %cmp, i8 2, i8 %y
  %cmp2 = icmp eq i8 %sel, %x
  ret i1 %cmp2
}

define i1 @sgt_3_impliesT_sgt_2(i8 %x, i8 %y) {
; CHECK-LABEL: @sgt_3_impliesT_sgt_2(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], 4
; CHECK-NEXT:    [[CMP21:%.*]] = icmp sgt i8 [[Y:%.*]], [[X]]
; CHECK-NEXT:    [[CMP2:%.*]] = select i1 [[CMP]], i1 [[CMP21]], i1 false
; CHECK-NEXT:    ret i1 [[CMP2]]
;
  %cmp = icmp sgt i8 %x, 3
  %sel = select i1 %cmp, i8 2, i8 %y
  %cmp2 = icmp sgt i8 %sel, %x
  ret i1 %cmp2
}

define i1 @sgt_x_impliesF_eq_smin_todo(i8 %x, i8 %y, i8 %z) {
; CHECK-LABEL: @sgt_x_impliesF_eq_smin_todo(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[X:%.*]], [[Z:%.*]]
; CHECK-NEXT:    [[CMP21:%.*]] = icmp eq i8 [[Y:%.*]], [[X]]
; CHECK-NEXT:    [[CMP2:%.*]] = select i1 [[CMP]], i1 [[CMP21]], i1 false
; CHECK-NEXT:    ret i1 [[CMP2]]
;
  %cmp = icmp sgt i8 %x, %z
  %sel = select i1 %cmp, i8 -128, i8 %y
  %cmp2 = icmp eq i8 %sel, %x
  ret i1 %cmp2
}

define i1 @slt_x_impliesT_ne_smin_todo(i8 %x, i8 %y, i8 %z) {
; CHECK-LABEL: @slt_x_impliesT_ne_smin_todo(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], [[Z:%.*]]
; CHECK-NEXT:    [[CMP21:%.*]] = icmp ne i8 [[Y:%.*]], [[X]]
; CHECK-NEXT:    [[CMP2:%.*]] = select i1 [[CMP]], i1 true, i1 [[CMP21]]
; CHECK-NEXT:    ret i1 [[CMP2]]
;
  %cmp = icmp slt i8 %x, %z
  %sel = select i1 %cmp, i8 127, i8 %y
  %cmp2 = icmp ne i8 %x, %sel
  ret i1 %cmp2
}

define i1 @ult_x_impliesT_eq_umax_todo(i8 %x, i8 %y, i8 %z) {
; CHECK-LABEL: @ult_x_impliesT_eq_umax_todo(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Z:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMP21:%.*]] = icmp ne i8 [[Y:%.*]], [[X]]
; CHECK-NEXT:    [[CMP2:%.*]] = select i1 [[CMP]], i1 true, i1 [[CMP21]]
; CHECK-NEXT:    ret i1 [[CMP2]]
;
  %cmp = icmp ugt i8 %z, %x
  %sel = select i1 %cmp, i8 255, i8 %y
  %cmp2 = icmp ne i8 %sel, %x
  ret i1 %cmp2
}

define i1 @ult_1_impliesF_eq_1(i8 %x, i8 %y) {
; CHECK-LABEL: @ult_1_impliesF_eq_1(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ne i8 [[X:%.*]], 0
; CHECK-NEXT:    [[CMP21:%.*]] = icmp eq i8 [[Y:%.*]], [[X]]
; CHECK-NEXT:    [[CMP2:%.*]] = select i1 [[CMP]], i1 [[CMP21]], i1 false
; CHECK-NEXT:    ret i1 [[CMP2]]
;
  %cmp = icmp ult i8 %x, 1
  %sel = select i1 %cmp, i8 1, i8 %y
  %cmp2 = icmp eq i8 %x, %sel
  ret i1 %cmp2
}

define i1 @ugt_x_impliesF_eq_umin_todo(i8 %x, i8 %y, i8 %z) {
; CHECK-LABEL: @ugt_x_impliesF_eq_umin_todo(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Z:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[SEL:%.*]] = select i1 [[CMP]], i8 0, i8 [[Y:%.*]]
; CHECK-NEXT:    [[CMP2:%.*]] = icmp eq i8 [[X]], [[SEL]]
; CHECK-NEXT:    ret i1 [[CMP2]]
;
  %cmp = icmp ugt i8 %z, %x
  %sel = select i1 %cmp, i8 0, i8 %y
  %cmp2 = icmp eq i8 %x, %sel
  ret i1 %cmp2
}
