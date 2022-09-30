#ifndef __MZTS_OUTPUTS_H
# define __MZTS_OUTPUTS_H


typedef struct
{
    volatile uint32_t  *reg_for_1;
    volatile uint32_t  *reg_for_0;
    uint32_t            bit;

} T_otputs_cfg;


// Управляющая структура машины состояний управляемой шаблоном
typedef struct
{
    uint32_t  init_state;
    uint32_t  counter;
    int32_t  *pattern_start_ptr;  // Указатель на массив констант являющийся цепочкой состояний (шаблоном)
                                  // Если значение в массиве = 0xFFFFFFFF, то процесс обработки завершается
                                  // Если значение в массиве = 0x00000000, то вернуть указатель на начало цепочки
    int32_t   *pttn_ptr;          // Текущая позиция в цепочке состояний

} T_outs_ptrn;

# define OUT_1WLED        7 // Сигнал светодиода считывателя 1WIRE. Зажигается 1

# define OUTPUTS_NUM      1 // Количество выходов

void    Set_output_state(uint8_t num, uint8_t val);
void    Outputs_set_pattern(const int32_t *pttn, uint32_t n);
void    Outputs_state_automat(void);
void    Outputs_set_period_ms(uint32_t val);

void    Set_output_blink(uint32_t out_num);
void    Set_output_on(uint32_t out_num);
void    Set_output_off(uint32_t out_num);
void    Set_output_blink_undef(uint32_t out_num);
void    Set_output_blink_3(uint32_t out_num);
void    Set_output_intf_active_blink(uint32_t out_num);
void    Set_output_off_blink_10(uint32_t out_num);
void    Outputs_clear_pattern(uint32_t n);

void    Set_LCD_RST            (int32_t v) ;
void    Set_LCD_DC             (int32_t v) ;
void    Set_LCD_BLK            (int32_t v) ;
void    Set_LCD_CS             (int32_t v) ;
void    Set_IMU_EN             (int32_t v) ;
void    Set_LSM_CS             (int32_t v) ;

#endif // OUTPUTS_PROCESSING_H



