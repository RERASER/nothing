#include <windows.h>

#include <limits.h>
#include <stdint.h>

#include "mercuryio/mercuryio.h"
#include "mercuryio/config.h"

static uint8_t mercury_opbtn;
static uint16_t mercury_player1_btn;
static uint16_t mercury_player2_btn;
static struct mercury_io_config mercury_io_cfg;

uint16_t mercury_io_get_api_version(void)
{
    return 0x0100;
}

HRESULT mercury_io_init(void)
{
    mercury_io_config_load(&mercury_io_cfg, L".\\segatools.ini");

    return S_OK;
}

HRESULT mercury_io_poll(void)
{
    mercury_opbtn = 0;
    mercury_player1_btn = 0;
    mercury_player2_btn = 0;

    if (GetAsyncKeyState(mercury_io_cfg.vk_test)) {
        mercury_opbtn |= MAI2_IO_OPBTN_TEST;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_service)) {
        mercury_opbtn |= MAI2_IO_OPBTN_SERVICE;
    }

    //Player 1
    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[0])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_1;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[1])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_2;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[2])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_3;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[3])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_4;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[4])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_5;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[5])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_6;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[6])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_7;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[7])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_8;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_1p_btn[8])) {
        mercury_player1_btn |= MAI2_IO_GAMEBTN_SELECT;
    }

    //Player 2
    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[0])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_1;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[1])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_2;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[2])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_3;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[3])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_4;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[4])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_5;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[5])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_6;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[6])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_7;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[7])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_8;
    }

    if (GetAsyncKeyState(mercury_io_cfg.vk_2p_btn[8])) {
        mercury_player2_btn |= MAI2_IO_GAMEBTN_SELECT;
    }

    return S_OK;
}

void mercury_io_get_opbtns(uint8_t *opbtn)
{
    if (opbtn != NULL) {
        *opbtn = mercury_opbtn;
    }
}

void mercury_io_get_gamebtns(uint16_t *player1, uint16_t *player2)
{
    if (player1 != NULL) {
        *player1 = mercury_player1_btn;
    }

    if (player2 != NULL ){
        *player2 = mercury_player2_btn;
    }
}