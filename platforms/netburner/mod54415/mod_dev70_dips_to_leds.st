(* MOD-DEV-70 demo: map 8 DIP switches to 8 LEDs (MPLC HAL channels 0..7).
 * Uses byte 0 bit addresses (%IX0.0..0.7 / %QX0.0..0.7). *)

VAR
    Dip1  AT %IX0.0 : BOOL;
    Dip2  AT %IX0.1 : BOOL;
    Dip3  AT %IX0.2 : BOOL;
    Dip4  AT %IX0.3 : BOOL;
    Dip5  AT %IX0.4 : BOOL;
    Dip6  AT %IX0.5 : BOOL;
    Dip7  AT %IX0.6 : BOOL;
    Dip8  AT %IX0.7 : BOOL;
    Led1  AT %QX0.0 : BOOL;
    Led2  AT %QX0.1 : BOOL;
    Led3  AT %QX0.2 : BOOL;
    Led4  AT %QX0.3 : BOOL;
    Led5  AT %QX0.4 : BOOL;
    Led6  AT %QX0.5 : BOOL;
    Led7  AT %QX0.6 : BOOL;
    Led8  AT %QX0.7 : BOOL;
END_VAR

PROGRAM Main
    Led1 := Dip1;
    Led2 := Dip2;
    Led3 := Dip3;
    Led4 := Dip4;
    Led5 := Dip5;
    Led6 := Dip6;
    Led7 := Dip7;
    Led8 := Dip8;
END_PROGRAM
