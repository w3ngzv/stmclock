#ifndef FASTMATH_H
#define FASTMATH_H

/* 本机工具链没有 libm, 以下两个函数用于姿态解算, 精度见 tests/test_logic.c */
float fm_sqrt(float x);
/* 返回 y/x 的反正切, 单位为度, 范围 (-180, 180] */
float fm_atan2_deg(float y, float x);
float fm_abs(float x);

#endif
