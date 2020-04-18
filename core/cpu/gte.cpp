#include <stdafx.hpp>
#include "gte.h"

GTE::GTE(CPU* _cpu)
{
    cpu = _cpu;

    register_opcodes();
}

uint GTE::read_control(uint reg)
{
    uint value;
    switch (reg) {
    case 00: value = RT.v1.xy; break;
    case 01: value = (ushort)RT.v1.z | (uint)(RT.v2.x << 16); break;
    case 02: value = (ushort)RT.v2.y | (uint)(RT.v2.z << 16); break;
    case 03: value = RT.v3.xy; break;
    case 04: value = (uint)RT.v3.z; break;
    case 05: value = (uint)TRX; break;
    case 06: value = (uint)TRY; break;
    case 07: value = (uint)TRZ; break;
    case 8: value = LM.v1.xy; break;
    case 9: value = (ushort)LM.v1.z | (uint)(LM.v2.x << 16); break;
    case 10: value = (ushort)LM.v2.y | (uint)(LM.v2.z << 16); break;
    case 11: value = LM.v3.xy; break;
    case 12: value = (uint)LM.v3.z; break;
    case 13: value = (uint)RBK; break;
    case 14: value = (uint)GBK; break;
    case 15: value = (uint)BBK; break;
    case 16: value = LRGB.v1.xy; break;
    case 17: value = (ushort)LRGB.v1.z | (uint)(LRGB.v2.x << 16); break;
    case 18: value = (ushort)LRGB.v2.y | (uint)(LRGB.v2.z << 16); break;
    case 19: value = LRGB.v3.xy; break;
    case 20: value = (uint)LRGB.v3.z; break;
    case 21: value = (uint)RFC; break;
    case 22: value = (uint)GFC; break;
    case 23: value = (uint)BFC; break;
    case 24: value = (uint)OFX; break;
    case 25: value = (uint)OFY; break;
    case 26: value = (uint)(short)H; break; //sign extend
    case 27: value = (uint)DQA; break;
    case 28: value = (uint)DQB; break;
    case 29: value = (uint)ZSF3; break; //sign extend
    case 30: value = (uint)ZSF4; break; //sign extend
    case 31: value = FLAG; break;
    default: value = 0xFFFFFFFF; break;
    }
    return value;
}

void GTE::interpolate(int32_t mac1, int32_t mac2, int32_t mac3)
{
    MAC1 = (int)(set_mac_flag(1, ((int64_t)RFC << 12) - mac1) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, ((int64_t)GFC << 12) - mac2) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, ((int64_t)BFC << 12) - mac3) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, false);
    IR[2] = set_ir_flag(2, MAC2, false);
    IR[3] = set_ir_flag(3, MAC3, false);

    MAC1 = (int)(set_mac_flag(1, ((int64_t)IR[1] * IR[0]) + mac1) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, ((int64_t)IR[2] * IR[0]) + mac2) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, ((int64_t)IR[3] * IR[0]) + mac3) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);
}

void GTE::execute(Instr instr)
{
    /* Convert to GTE command format. */
    command.raw = instr.value;

    auto& handler = lookup[command.opcode];
    
    if (handler != nullptr)
        handler();
    else {
        std::cout << "Unhandled GTE command: 0x" << std::hex << command.opcode << '\n';
        exit(0);
    }
}

Matrix GTE::get_mvmva_matrix()
{
    switch (command.mvmva_mul_matrix) {
    case 0: return RT;
    case 1: return LM;
    case 2: return LRGB;
    default: exit(0);
    }
}

Vec3i16 GTE::get_mvmva_vector()
{
    Vec3i16 value;
    switch (command.mvmva_mul_vec) {
    case 0: value = V[0]; break;
    case 1: value = V[1]; break;
    case 2: value = V[2]; break;
    case 3: value.vector = { IR[1], IR[2], IR[3] }; break;
    }

    return value;
}

std::tuple<int, int, int> GTE::get_mvmva_translation()
{
    switch (command.mvmva_tr_vec) {
    case 0: return std::make_tuple(TRX, TRY, TRZ);
    case 1: return std::make_tuple(RBK, GBK, BBK);
    case 2: return std::make_tuple(RFC, GFC, BFC);
    case 3: return std::make_tuple(0, 0, 0);
    default: exit(0);
    }
}

uint GTE::read_data(uint reg)
{
    //std::cout << "GTE Register read: " << reg << '\n';

    auto saturateRGB = [&](int v) -> short
    {
        short saturate = (short)v;
        if (saturate < 0x00) return 0x00;
        else if (saturate > 0x1F) return 0x1F;
        else return saturate;
    };

    uint value;
    switch (reg) {
    case 00: value = V[0].xy; break;
    case 01: value = (uint)V[0].z; break;
    case 02: value = V[1].xy; break;
    case 03: value = (uint)V[1].z; break;
    case 04: value = V[2].xy; break;
    case 05: value = (uint)V[2].z; break;
    case 06: value = RGBC.val; break;
    case 07: value = OTZ; break;
    case 8: value = (uint)IR[0]; break;
    case 9: value = (uint)IR[1]; break;
    case 10: value = (uint)IR[2]; break;
    case 11: value = (uint)IR[3]; break;
    case 12: value = SXY[0].val; break;
    case 13: value = SXY[1].val; break;
    case 14: //Mirror
    case 15: value = SXY[2].val; break;
    case 16: value = SZ[0]; break;
    case 17: value = SZ[1]; break;
    case 18: value = SZ[2]; break;
    case 19: value = SZ[3]; break;
    case 20: value = RGB[0].val; break;
    case 21: value = RGB[1].val; break;
    case 22: value = RGB[2].val; break;
    case 23: value = RES1; break; //Prohibited Register
    case 24: value = (uint)MAC0; break;
    case 25: value = (uint)MAC1; break;
    case 26: value = (uint)MAC2; break;
    case 27: value = (uint)MAC3; break;
    case 28:/* value = IRGB; break;*/
    case 29:/* value = ORGB; break;*/
        IRGB = (ushort)(saturateRGB(IR[3] / 0x80) << 10 | saturateRGB(IR[2] / 0x80) << 5 | (ushort)saturateRGB(IR[1] / 0x80));
        value = IRGB;
        break;
    case 30: value = (uint)LZCS; break;
    case 31: value = (uint)LZCR; break;
    default: value = 0xFFFFFFFF; break;
    }
    return value;
}

void GTE::write_data(uint reg, uint v)
{
    // std::cout << "GTE Register write: " << reg;
     //std::cout << " with data: " << v << '\n';

    auto leadingCount = [&](uint v) -> int
    {
        uint sign = (v >> 31);
        int leadingCount = 0;
        for (int i = 0; i < 32; i++) {
            if (v >> 31 != sign) break;
            leadingCount++;
            v <<= 1;
        }
        return leadingCount;
    };

    switch (reg) {
    case 00: V[0].xy = v; break;
    case 01: V[0].z = (short)v; break;
    case 02: V[1].xy = v; break;
    case 03: V[1].z = (short)v; break;
    case 04: V[2].xy = v; break;
    case 05: V[2].z = (short)v; break;
    case 06: RGBC.val = v; break;
    case 07: OTZ = (ushort)v; break;
    case 8: IR[0] = (short)v; break;
    case 9: IR[1] = (short)v; break;
    case 10: IR[2] = (short)v; break;
    case 11: IR[3] = (short)v; break;
    case 12: SXY[0].val = v; break;
    case 13: SXY[1].val = v; break;
    case 14: SXY[2].val = v; break;
    case 15: SXY[0] = SXY[1]; SXY[1] = SXY[2]; SXY[2].val = v; break; //On load mirrors 0x14 on write cycles the fifo
    case 16: SZ[0] = (ushort)v; break;
    case 17: SZ[1] = (ushort)v; break;
    case 18: SZ[2] = (ushort)v; break;
    case 19: SZ[3] = (ushort)v; break;
    case 20: RGB[0].val = v; break;
    case 21: RGB[1].val = v; break;
    case 22: RGB[2].val = v; break;
    case 23: RES1 = v; break;
    case 24: MAC0 = (int)v; break;
    case 25: MAC1 = (int)v; break;
    case 26: MAC2 = (int)v; break;
    case 27: MAC3 = (int)v; break;
    case 28:
        IRGB = (ushort)(v & 0x7FFF);
        IR[1] = (short)((v & 0x1F) * 0x80);
        IR[2] = (short)(((v >> 5) & 0x1F) * 0x80);
        IR[3] = (short)(((v >> 10) & 0x1F) * 0x80);
        break;
    case 29: /*ORGB = (ushort)v;*/ break; //Only Read its set by IRGB
    case 30: LZCS = (int)v; LZCR = leadingCount(v); break;
    case 31: /*LZCR = (int)v;*/ break; //Only Read its set by LZCS
    }
}

void GTE::write_control(uint reg, uint v)
{
    switch (reg) {
    case 00: RT.v1.xy = v; break;
    case 01: RT.v1.z = (short)v; RT.v2.x = (short)(v >> 16); break;
    case 02: RT.v2.y = (short)v; RT.v2.z = (short)(v >> 16); break;
    case 03: RT.v3.xy = v; break;
    case 04: RT.v3.z = (short)v; break;
    case 05: TRX = (int)v; break;
    case 06: TRY = (int)v; break;
    case 07: TRZ = (int)v; break;
    case 8: LM.v1.xy = v; break;
    case 9: LM.v1.z = (short)v; LM.v2.x = (short)(v >> 16); break;
    case 10: LM.v2.y = (short)v; LM.v2.z = (short)(v >> 16); break;
    case 11: LM.v3.xy = v; break;
    case 12: LM.v3.z = (short)v; break;
    case 13: RBK = (int)v; break;
    case 14: GBK = (int)v; break;
    case 15: BBK = (int)v; break;
    case 16: LRGB.v1.xy = v; break;
    case 17: LRGB.v1.z = (short)v; LRGB.v2.x = (short)(v >> 16); break;
    case 18: LRGB.v2.y = (short)v; LRGB.v2.z = (short)(v >> 16); break;
    case 19: LRGB.v3.xy = v; break;
    case 20: LRGB.v3.z = (short)v; break;
    case 21: RFC = (int)v; break;
    case 22: GFC = (int)v; break;
    case 23: BFC = (int)v; break;
    case 24: OFX = (int)v; break;
    case 25: OFY = (int)v; break;
    case 26: H = (ushort)v; break;
    case 27: DQA = (short)v; break;
    case 28: DQB = (int)v; break;
    case 29: ZSF3 = (short)v; break;
    case 30: ZSF4 = (short)v; break;
    case 31: //flag is u20 with 31 Error Flag (Bit30..23, and 18..13 ORed together)
        FLAG = v & 0x7FFFF000;
        if ((FLAG & 0x7F87E000) != 0) {
            FLAG |= 0x80000000;
        }
        break;
    }
}

inline int64_t GTE::set_mac_flag(int i, int64_t value)
{
    if (value < -0x80000000000) {
        //Console.WriteLine("under");
        FLAG |= (uint)(0x8000000 >> (i - 1));
    }

    if (value > 0x7FFFFFFFFFF) {
        //Console.WriteLine("over");
        FLAG |= (uint)(0x40000000 >> (i - 1));
    }

    return (value << 20) >> 20;
}

inline int32_t GTE::set_mac0_flag(int64_t value)
{
    if (value < -0x80000000LL) {
        FLAG |= 0x8000;
    }
    else if (value > 0x7FFFFFFF) {
        FLAG |= 0x10000;
    }
    return (int)value;
}

inline ushort GTE::set_sz3_flag(int64_t value)
{
    if (value < 0) {
        FLAG |= 0x40000;
        return 0;
    }

    if (value > 0xFFFF) {
        FLAG |= 0x40000;
        return 0xFFFF;
    }

    return (ushort)value;
}

inline int16_t GTE::set_sxy_flag(int i, int32_t value)
{
    if (value < -0x400) {
        FLAG |= (uint)(0x4000 >> (i - 1));
        return -0x400;
    }

    if (value > 0x3FF) {
        FLAG |= (uint)(0x4000 >> (i - 1));
        return 0x3FF;
    }

    return (short)value;
}

inline int16_t GTE::set_ir0_flag(int64_t value)
{
    if (value < 0) {
        FLAG |= 0x1000;
        return 0;
    }

    if (value > 0x1000) {
        FLAG |= 0x1000;
        return 0x1000;
    }

    return (short)value;
}

inline int16_t GTE::set_ir_flag(int i, int32_t value, bool lm)
{
    if (lm && value < 0) {
        FLAG |= (uint)(0x1000000 >> (i - 1));
        return 0;
    }

    if (!lm && (value < -0x8000)) {
        FLAG |= (uint)(0x1000000 >> (i - 1));
        return -0x8000;
    }

    if (value > 0x7FFF) {
        FLAG |= (uint)(0x1000000 >> (i - 1));
        return 0x7FFF;
    }

    return (short)value;
}

ubyte GTE::set_rgb(int i, int value) {
    if (value < 0) {
        FLAG |= (uint)0x200000 >> (i - 1);
        return 0;
    }

    if (value > 0xFF) {
        FLAG |= (uint)0x200000 >> (i - 1);
        return 0xFF;
    }

    return (ubyte)value;
}

void GTE::op_rtps()
{
    int i = 0;
    
    MAC1 = (int)set_mac_flag(1, (TRX * 0x1000 + RT.v1.x * V[i].x + RT.v1.y * V[i].y + RT.v1.z * V[i].z) >> (command.sf * 12));
    MAC2 = (int)set_mac_flag(2, (TRY * 0x1000 + RT.v2.x * V[i].x + RT.v2.y * V[i].y + RT.v2.z * V[i].z) >> (command.sf * 12));
    MAC3 = (int)set_mac_flag(3, (TRZ * 0x1000 + RT.v3.x * V[i].x + RT.v3.y * V[i].y + RT.v3.z * V[i].z) >> (command.sf * 12));

    IR[1] = set_ir_flag(1, MAC1, false);
    IR[2] = set_ir_flag(2, MAC2, false);
    IR[3] = set_ir_flag(3, MAC3, false);

    //SZ3 = MAC3 SAR ((1-command.)*12)                           ;ScreenZ FIFO 0..+FFFFh

    SZ[0] = SZ[1];
    SZ[1] = SZ[2];
    SZ[2] = SZ[3];
    SZ[3] = set_sz3_flag(MAC3 >> ((1 - command.sf) * 12));

    SXY[0] = SXY[1];
    SXY[1] = SXY[2];

    int64_t result;
    int64_t div = 0;
    if (SZ[3] == 0) {
        result = 0x1FFFF;
    }
    else {
        div = (((int64_t)H * 0x20000 / SZ[3]) + 1) / 2;

        if (div > 0x1FFFF) {
            result = 0x1FFFF;
            FLAG |= 0x1 << 17;
        }
        else {
            result = div;
        }
    }

    MAC0 = (int)(result * IR[1] + OFX);
    SXY[2].x = set_sxy_flag(2, MAC0 / 0x10000);
    MAC0 = (int)(result * IR[2] + OFY);
    SXY[2].y = set_sxy_flag(2, MAC0 / 0x10000);
    MAC0 = (int)(result * DQA + DQB);
    IR[0] = set_ir0_flag(MAC0 / 0x1000);
}

void GTE::op_rtpt()
{
    for (int i = 0; i < 3; i++) {
        MAC1 = (int)set_mac_flag(1, (TRX * 0x1000 + RT.v1.x * V[i].x + RT.v1.y * V[i].y + RT.v1.z * V[i].z) >> (command.sf * 12));
        MAC2 = (int)set_mac_flag(2, (TRY * 0x1000 + RT.v2.x * V[i].x + RT.v2.y * V[i].y + RT.v2.z * V[i].z) >> (command.sf * 12));
        MAC3 = (int)set_mac_flag(3, (TRZ * 0x1000 + RT.v3.x * V[i].x + RT.v3.y * V[i].y + RT.v3.z * V[i].z) >> (command.sf * 12));

        IR[1] = set_ir_flag(1, MAC1, false);
        IR[2] = set_ir_flag(2, MAC2, false);
        IR[3] = set_ir_flag(3, MAC3, false);

        //SZ3 = MAC3 SAR ((1-command.)*12)                           ;ScreenZ FIFO 0..+FFFFh

        SZ[0] = SZ[1];
        SZ[1] = SZ[2];
        SZ[2] = SZ[3];
        SZ[3] = set_sz3_flag(MAC3 >> ((1 - command.sf) * 12));

        //MAC0=(((H*20000h/SZ3)+1)/2)*IR1+OFX, SX2=MAC0/10000h ;ScrX FIFO -400h..+3FFh
        //MAC0=(((H*20000h/SZ3)+1)/2)*IR2+OFY, SY2=MAC0/10000h ;ScrY FIFO -400h..+3FFh
        //MAC0=(((H*20000h/SZ3)+1)/2)*DQA+DQB, IR0=MAC0/1000h  ;Depth cueing 0..+1000h

        SXY[0] = SXY[1];
        SXY[1] = SXY[2];

        int64_t result;
        int64_t div = 0;
        if (SZ[3] == 0) {
            result = 0x1FFFF;
        }
        else {
            div = (((int64_t)H * 0x20000 / SZ[3]) + 1) / 2;

            if (div > 0x1FFFF) {
                result = 0x1FFFF;
                FLAG |= 0x1 << 17;
            }
            else {
                result = div;
            }
        }

        MAC0 = (int)(result * IR[1] + OFX);
        SXY[2].x = set_sxy_flag(2, MAC0 / 0x10000);
        MAC0 = (int)(result * IR[2] + OFY);
        SXY[2].y = set_sxy_flag(2, MAC0 / 0x10000);
        MAC0 = (int)(result * DQA + DQB);
        IR[0] = set_ir0_flag(MAC0 / 0x1000);
    }
}

void GTE::op_mvmva()
{
    Matrix mx = get_mvmva_matrix();
    Vec3i16 vx = get_mvmva_vector();
    auto [tx, ty, tz] = get_mvmva_translation();

    MAC1 = (int)set_mac_flag(1, (int64_t)(tx * 0x1000 + mx.v1.x * vx.x + mx.v1.y * vx.y + mx.v1.z * vx.z) >> command.sf * 12);
    MAC2 = (int)set_mac_flag(2, (int64_t)(ty * 0x1000 + mx.v2.x * vx.x + mx.v2.y * vx.y + mx.v2.z * vx.z) >> command.sf * 12);
    MAC3 = (int)set_mac_flag(3, (int64_t)(tz * 0x1000 + mx.v3.x * vx.x + mx.v3.y * vx.y + mx.v3.z * vx.z) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);
}

void GTE::op_dcpl()
{
}

void GTE::op_dpcs()
{
    ubyte r = RGBC.r;
    ubyte g = RGBC.g;
    ubyte b = RGBC.b;

    // WHEN DCPT it uses RGB FIFO instead RGBC
    //[MAC1, MAC2, MAC3] = [R, G, B] SHL 16                     ;<--- for DPCS/DPCT
    MAC1 = (int)(set_mac_flag(1, r) << 16);
    MAC2 = (int)(set_mac_flag(2, g) << 16);
    MAC3 = (int)(set_mac_flag(3, b) << 16);

    interpolate(MAC1, MAC2, MAC3);

    // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE]
    RGB[0] = RGB[1];
    RGB[1] = RGB[2];

    RGB[2].r = set_rgb(1, MAC1 >> 4);
    RGB[2].g = set_rgb(2, MAC2 >> 4);
    RGB[2].b = set_rgb(3, MAC3 >> 4);
    RGB[2].c = RGBC.c;
}

void GTE::op_intpl()
{
    MAC1 = (int)set_mac_flag(1, (int64_t)IR[1] << 12);
    MAC2 = (int)set_mac_flag(2, (int64_t)IR[2] << 12);
    MAC3 = (int)set_mac_flag(3, (int64_t)IR[3] << 12);

    interpolate(MAC1, MAC2, MAC3);

    // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE]
    RGB[0] = RGB[1];
    RGB[1] = RGB[2];

    RGB[2].r = set_rgb(1, MAC1 >> 4);
    RGB[2].g = set_rgb(2, MAC2 >> 4);
    RGB[2].b = set_rgb(3, MAC3 >> 4);
    RGB[2].c = RGBC.c;
}

void GTE::op_ncdt()
{
    for (int r = 0; r < 3; r++) {
        MAC1 = (int)(set_mac_flag(1, (int64_t)LM.v1.x * V[r].x + LM.v1.y * V[r].y + LM.v1.z * V[r].z) >> command.sf * 12);
        MAC2 = (int)(set_mac_flag(2, (int64_t)LM.v2.x * V[r].x + LM.v2.y * V[r].y + LM.v2.z * V[r].z) >> command.sf * 12);
        MAC3 = (int)(set_mac_flag(3, (int64_t)LM.v3.x * V[r].x + LM.v3.y * V[r].y + LM.v3.z * V[r].z) >> command.sf * 12);

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);

        //Console.WriteLine("NCDS " + ncdsTest + " " + MAC1.ToString("x8") + " " + MAC2.ToString("x8") + " " + MAC3.ToString("x8") + " " + (command.sf * 12).ToString("x1")
        //             + " " + IR[0].ToString("x4") + " " + IR[1].ToString("x4") + " " + IR[2].ToString("x4") + " " + IR[3].ToString("x4") + " " + FLAG.ToString("x8"));

        // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (BK * 1000h + LCM * IR) SAR(command.sf * 12)
        // WARNING each multiplication can trigger mac flags so the check is needed on each op! Somehow this only affects the color matrix and not the light one
        MAC1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (int64_t)RBK * 0x1000 + LRGB.v1.x * IR[1]) + (int64_t)LRGB.v1.y * IR[2]) + (int64_t)LRGB.v1.z * IR[3]) >> command.sf * 12);
        MAC2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (int64_t)GBK * 0x1000 + LRGB.v2.x * IR[1]) + (int64_t)LRGB.v2.y * IR[2]) + (int64_t)LRGB.v2.z * IR[3]) >> command.sf * 12);
        MAC3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (int64_t)BBK * 0x1000 + LRGB.v3.x * IR[1]) + (int64_t)LRGB.v3.y * IR[2]) + (int64_t)LRGB.v3.z * IR[3]) >> command.sf * 12);

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);

        // [MAC1, MAC2, MAC3] = [R * IR1, G * IR2, B * IR3] SHL 4;< --- for NCDx / NCCx
        MAC1 = (int)set_mac_flag(1, ((int64_t)RGBC.r * IR[1]) << 4);
        MAC2 = (int)set_mac_flag(2, ((int64_t)RGBC.g * IR[2]) << 4);
        MAC3 = (int)set_mac_flag(3, ((int64_t)RGBC.b * IR[3]) << 4);

        interpolate(MAC1, MAC2, MAC3);

        // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE]
        RGB[0] = RGB[1];
        RGB[1] = RGB[2];

        RGB[2].r = set_rgb(1, MAC1 >> 4);
        RGB[2].g = set_rgb(2, MAC2 >> 4);
        RGB[2].b = set_rgb(3, MAC3 >> 4);
        RGB[2].c = RGBC.c;
    }
}

void GTE::op_sqr()
{
    MAC1 = (int)set_mac_flag(1, (IR[1] * IR[1]) >> command.sf * 12);
    MAC2 = (int)set_mac_flag(2, (IR[2] * IR[2]) >> command.sf * 12);
    MAC3 = (int)set_mac_flag(3, (IR[3] * IR[3]) >> command.sf * 12);


    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);
}

void GTE::op_nct()
{
    for (int r = 0; r < 3; r++) {
        MAC1 = (int)(set_mac_flag(1, (long long)LM.v1.x * V[r].x + LM.v1.y * V[r].y + LM.v1.z * V[r].z) >> command.sf * 12);
        MAC2 = (int)(set_mac_flag(2, (long long)LM.v2.x * V[r].x + LM.v2.y * V[r].y + LM.v2.z * V[r].z) >> command.sf * 12);
        MAC3 = (int)(set_mac_flag(3, (long long)LM.v3.x * V[r].x + LM.v3.y * V[r].y + LM.v3.z * V[r].z) >> command.sf * 12);

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);

        // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (BK * 1000h + LCM * IR) SAR(sf * 12)
        // WARNING each multiplication can trigger mac flags so the check is needed on each op! Somehow this only affects the color matrix and not the light one
        MAC1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (long long)RBK * 0x1000 + LRGB.v1.x * IR[1]) + (long long)LRGB.v1.y * IR[2]) + (long long)LRGB.v1.z * IR[3]) >> command.sf * 12);
        MAC2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (long long)GBK * 0x1000 + LRGB.v2.x * IR[1]) + (long long)LRGB.v2.y * IR[2]) + (long long)LRGB.v2.z * IR[3]) >> command.sf * 12);
        MAC3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (long long)BBK * 0x1000 + LRGB.v3.x * IR[1]) + (long long)LRGB.v3.y * IR[2]) + (long long)LRGB.v3.z * IR[3]) >> command.sf * 12);

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);

        // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE], [IR1, IR2, IR3] = [MAC1, MAC2, MAC3]
        RGB[0] = RGB[1];
        RGB[1] = RGB[2];

        RGB[2].r = set_rgb(1, MAC1 >> 4);
        RGB[2].g = set_rgb(2, MAC2 >> 4);
        RGB[2].b = set_rgb(3, MAC3 >> 4);
        RGB[2].c = RGBC.c;

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);
    }
}

void GTE::op_ncs()
{
    int r = 0;
    MAC1 = (int)(set_mac_flag(1, (long long)LM.v1.x * V[r].x + LM.v1.y * V[r].y + LM.v1.z * V[r].z) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, (long long)LM.v2.x * V[r].x + LM.v2.y * V[r].y + LM.v2.z * V[r].z) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, (long long)LM.v3.x * V[r].x + LM.v3.y * V[r].y + LM.v3.z * V[r].z) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (BK * 1000h + LCM * IR) SAR(sf * 12)
    // WARNING each multiplication can trigger mac flags so the check is needed on each op! Somehow this only affects the color matrix and not the light one
    MAC1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (long long)RBK * 0x1000 + LRGB.v1.x * IR[1]) + (long long)LRGB.v1.y * IR[2]) + (long long)LRGB.v1.z * IR[3]) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (long long)GBK * 0x1000 + LRGB.v2.x * IR[1]) + (long long)LRGB.v2.y * IR[2]) + (long long)LRGB.v2.z * IR[3]) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (long long)BBK * 0x1000 + LRGB.v3.x * IR[1]) + (long long)LRGB.v3.y * IR[2]) + (long long)LRGB.v3.z * IR[3]) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE], [IR1, IR2, IR3] = [MAC1, MAC2, MAC3]
    RGB[0] = RGB[1];
    RGB[1] = RGB[2];

    RGB[2].r = set_rgb(1, MAC1 >> 4);
    RGB[2].g = set_rgb(2, MAC2 >> 4);
    RGB[2].b = set_rgb(3, MAC3 >> 4);
    RGB[2].c = RGBC.c;

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);
}

void GTE::op_ncds()
{
    MAC1 = (int)(set_mac_flag(1, (int64_t)LM.v1.x * V[0].x + LM.v1.y * V[0].y + LM.v1.z * V[0].z) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, (int64_t)LM.v2.x * V[0].x + LM.v2.y * V[0].y + LM.v2.z * V[0].z) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, (int64_t)LM.v3.x * V[0].x + LM.v3.y * V[0].y + LM.v3.z * V[0].z) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    //Console.WriteLine("NCDS " + ncdsTest + " " + MAC1.ToString("x8") + " " + MAC2.ToString("x8") + " " + MAC3.ToString("x8") + " " + (command.sf * 12).ToString("x1")
    //             + " " + IR[0].ToString("x4") + " " + IR[1].ToString("x4") + " " + IR[2].ToString("x4") + " " + IR[3].ToString("x4") + " " + FLAG.ToString("x8"));

    // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (BK * 1000h + LCM * IR) SAR(command.sf * 12)
    // WARNING each multiplication can trigger mac flags so the check is needed on each op! Somehow this only affects the color matrix and not the light one
    MAC1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (int64_t)RBK * 0x1000 + LRGB.v1.x * IR[1]) + (int64_t)LRGB.v1.y * IR[2]) + (int64_t)LRGB.v1.z * IR[3]) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (int64_t)GBK * 0x1000 + LRGB.v2.x * IR[1]) + (int64_t)LRGB.v2.y * IR[2]) + (int64_t)LRGB.v2.z * IR[3]) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (int64_t)BBK * 0x1000 + LRGB.v3.x * IR[1]) + (int64_t)LRGB.v3.y * IR[2]) + (int64_t)LRGB.v3.z * IR[3]) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    // [MAC1, MAC2, MAC3] = [R * IR1, G * IR2, B * IR3] SHL 4;< --- for NCDx / NCCx
    MAC1 = (int)set_mac_flag(1, ((int64_t)RGBC.r * IR[1]) << 4);
    MAC2 = (int)set_mac_flag(2, ((int64_t)RGBC.g * IR[2]) << 4);
    MAC3 = (int)set_mac_flag(3, ((int64_t)RGBC.b * IR[3]) << 4);

    interpolate(MAC1, MAC2, MAC3);

    // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE]
    RGB[0] = RGB[1];
    RGB[1] = RGB[2];

    RGB[2].r = set_rgb(1, MAC1 >> 4);
    RGB[2].g = set_rgb(2, MAC2 >> 4);
    RGB[2].b = set_rgb(3, MAC3 >> 4);
    RGB[2].c = RGBC.c;
}

void GTE::op_nccs()
{
    int r = 0;
    MAC1 = (int)(set_mac_flag(1, (int64_t)LM.v1.x * V[r].x + LM.v1.y * V[r].y + LM.v1.z * V[r].z) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, (int64_t)LM.v2.x * V[r].x + LM.v2.y * V[r].y + LM.v2.z * V[r].z) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, (int64_t)LM.v3.x * V[r].x + LM.v3.y * V[r].y + LM.v3.z * V[r].z) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (BK * 1000h + LCM * IR) SAR(command.sf * 12)
    // WARNING each multiplication can trigger mac flags so the check is needed on each op! Somehow this only affects the color matrix and not the light one
    MAC1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (int64_t)RBK * 0x1000 + LRGB.v1.x * IR[1]) + (int64_t)LRGB.v1.y * IR[2]) + (int64_t)LRGB.v1.z * IR[3]) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (int64_t)GBK * 0x1000 + LRGB.v2.x * IR[1]) + (int64_t)LRGB.v2.y * IR[2]) + (int64_t)LRGB.v2.z * IR[3]) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (int64_t)BBK * 0x1000 + LRGB.v3.x * IR[1]) + (int64_t)LRGB.v3.y * IR[2]) + (int64_t)LRGB.v3.z * IR[3]) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    // [MAC1, MAC2, MAC3] = [R * IR1, G * IR2, B * IR3] SHL 4;< --- for NCDx / NCCx
    MAC1 = (int)set_mac_flag(1, (RGBC.r * IR[1]) << 4);
    MAC2 = (int)set_mac_flag(2, (RGBC.g * IR[2]) << 4);
    MAC3 = (int)set_mac_flag(3, (RGBC.b * IR[3]) << 4);

    // [MAC1, MAC2, MAC3] = [MAC1, MAC2, MAC3] SAR(command.sf * 12);< --- for NCDx / NCCx
    MAC1 = (int)set_mac_flag(1, MAC1 >> command.sf * 12);
    MAC2 = (int)set_mac_flag(2, MAC2 >> command.sf * 12);
    MAC3 = (int)set_mac_flag(3, MAC3 >> command.sf * 12);

    // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE], [IR1, IR2, IR3] = [MAC1, MAC2, MAC3]
    RGB[0] = RGB[1];
    RGB[1] = RGB[2];

    RGB[2].r = set_rgb(1, MAC1 >> 4);
    RGB[2].g = set_rgb(2, MAC2 >> 4);
    RGB[2].b = set_rgb(3, MAC3 >> 4);
    RGB[2].c = RGBC.c;

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);
}

void GTE::op_ncct()
{
    for (int r = 0; r < 3; r++) {
        MAC1 = (int)(set_mac_flag(1, (int64_t)LM.v1.x * V[r].x + LM.v1.y * V[r].y + LM.v1.z * V[r].z) >> command.sf * 12);
        MAC2 = (int)(set_mac_flag(2, (int64_t)LM.v2.x * V[r].x + LM.v2.y * V[r].y + LM.v2.z * V[r].z) >> command.sf * 12);
        MAC3 = (int)(set_mac_flag(3, (int64_t)LM.v3.x * V[r].x + LM.v3.y * V[r].y + LM.v3.z * V[r].z) >> command.sf * 12);

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);

        // [IR1, IR2, IR3] = [MAC1, MAC2, MAC3] = (BK * 1000h + LCM * IR) SAR(command.sf * 12)
        // WARNING each multiplication can trigger mac flags so the check is needed on each op! Somehow this only affects the color matrix and not the light one
        MAC1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (int64_t)RBK * 0x1000 + LRGB.v1.x * IR[1]) + (int64_t)LRGB.v1.y * IR[2]) + (int64_t)LRGB.v1.z * IR[3]) >> command.sf * 12);
        MAC2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (int64_t)GBK * 0x1000 + LRGB.v2.x * IR[1]) + (int64_t)LRGB.v2.y * IR[2]) + (int64_t)LRGB.v2.z * IR[3]) >> command.sf * 12);
        MAC3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (int64_t)BBK * 0x1000 + LRGB.v3.x * IR[1]) + (int64_t)LRGB.v3.y * IR[2]) + (int64_t)LRGB.v3.z * IR[3]) >> command.sf * 12);

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);

        // [MAC1, MAC2, MAC3] = [R * IR1, G * IR2, B * IR3] SHL 4;< --- for NCDx / NCCx
        MAC1 = (int)set_mac_flag(1, (RGBC.r * IR[1]) << 4);
        MAC2 = (int)set_mac_flag(2, (RGBC.g * IR[2]) << 4);
        MAC3 = (int)set_mac_flag(3, (RGBC.b * IR[3]) << 4);

        // [MAC1, MAC2, MAC3] = [MAC1, MAC2, MAC3] SAR(command.sf * 12);< --- for NCDx / NCCx
        MAC1 = (int)set_mac_flag(1, MAC1 >> command.sf * 12);
        MAC2 = (int)set_mac_flag(2, MAC2 >> command.sf * 12);
        MAC3 = (int)set_mac_flag(3, MAC3 >> command.sf * 12);

        // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE], [IR1, IR2, IR3] = [MAC1, MAC2, MAC3]
        RGB[0] = RGB[1];
        RGB[1] = RGB[2];

        RGB[2].r = set_rgb(1, MAC1 >> 4);
        RGB[2].g = set_rgb(2, MAC2 >> 4);
        RGB[2].b = set_rgb(3, MAC3 >> 4);
        RGB[2].c = RGBC.c;

        IR[1] = set_ir_flag(1, MAC1, command.lm);
        IR[2] = set_ir_flag(2, MAC2, command.lm);
        IR[3] = set_ir_flag(3, MAC3, command.lm);
    }
}

void GTE::op_cdp()
{
}

void GTE::op_cc()
{
    MAC1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (int64_t)RBK * 0x1000 + LRGB.v1.x * IR[1]) + (int64_t)LRGB.v1.y * IR[2]) + (int64_t)LRGB.v1.z * IR[3]) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (int64_t)GBK * 0x1000 + LRGB.v2.x * IR[1]) + (int64_t)LRGB.v2.y * IR[2]) + (int64_t)LRGB.v2.z * IR[3]) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (int64_t)BBK * 0x1000 + LRGB.v3.x * IR[1]) + (int64_t)LRGB.v3.y * IR[2]) + (int64_t)LRGB.v3.z * IR[3]) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    // [MAC1, MAC2, MAC3] = [R * IR1, G * IR2, B * IR3] SHL 4;
    MAC1 = (int)(set_mac_flag(1, (int64_t)RGBC.r * IR[1]) << 4);
    MAC2 = (int)(set_mac_flag(2, (int64_t)RGBC.g * IR[2]) << 4);
    MAC3 = (int)(set_mac_flag(3, (int64_t)RGBC.b * IR[3]) << 4);

    // [MAC1, MAC2, MAC3] = [MAC1, MAC2, MAC3] SAR(command.sf * 12);< --- for NCDx / NCCx
    MAC1 = (int)(set_mac_flag(1, MAC1) >> command.sf * 12);
    MAC2 = (int)(set_mac_flag(2, MAC2) >> command.sf * 12);
    MAC3 = (int)(set_mac_flag(3, MAC3) >> command.sf * 12);

    // Color FIFO = [MAC1 / 16, MAC2 / 16, MAC3 / 16, CODE], [IR1, IR2, IR3] = [MAC1, MAC2, MAC3]
    RGB[0] = RGB[1];
    RGB[1] = RGB[2];

    RGB[2].r = set_rgb(1, MAC1 >> 4);
    RGB[2].g = set_rgb(2, MAC2 >> 4);
    RGB[2].b = set_rgb(3, MAC3 >> 4);
    RGB[2].c = RGBC.c;

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);
}

void GTE::op_nclip()
{
    MAC0 = set_mac0_flag((int64_t)SXY[0].x * SXY[1].y + SXY[1].x * SXY[2].y + SXY[2].x * SXY[0].y - SXY[0].x * SXY[2].y - SXY[1].x * SXY[0].y - SXY[2].x * SXY[1].y);
}

void GTE::op_avsz3()
{
    //std::cout << "GTE AVSZ3 command\n";

    /* Compute the average of the z values. */
    int64_t avsz3 = (int64_t)ZSF3 * (SZ[1] + SZ[2] + SZ[3]);
    MAC0 = set_mac0_flag(avsz3);
    OTZ = set_sz3_flag(avsz3 >> 12);
}

void GTE::op_avsz4()
{
    long long avsz4 = (long long)ZSF4 * (SZ[0] + SZ[1] + SZ[2] + SZ[3]);
    MAC0 = set_mac0_flag(avsz4);
    OTZ = set_sz3_flag(avsz4 >> 12);
}

void GTE::op_op()
{
    short d1 = RT.v1.x;
    short d2 = RT.v2.y;
    short d3 = RT.v3.z;

    MAC1 = (int)set_mac_flag(1, ((IR[3] * d2) - (IR[2] * d3)) >> command.sf * 12);
    MAC2 = (int)set_mac_flag(2, ((IR[1] * d3) - (IR[3] * d1)) >> command.sf * 12);
    MAC3 = (int)set_mac_flag(3, ((IR[2] * d1) - (IR[1] * d2)) >> command.sf * 12);

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);
}

void GTE::op_gpf()
{
    MAC1 = (int)set_mac_flag(1, IR[1] * IR[0]) >> command.sf * 12;
    MAC2 = (int)set_mac_flag(2, IR[2] * IR[0]) >> command.sf * 12;
    MAC3 = (int)set_mac_flag(3, IR[3] * IR[0]) >> command.sf * 12;

    IR[1] = set_ir_flag(1, MAC1, command.lm);
    IR[2] = set_ir_flag(2, MAC2, command.lm);
    IR[3] = set_ir_flag(3, MAC3, command.lm);

    RGB[0] = RGB[1];
    RGB[1] = RGB[2];

    RGB[2].r = set_rgb(1, MAC1 >> 4);
    RGB[2].g = set_rgb(2, MAC2 >> 4);
    RGB[2].b = set_rgb(3, MAC3 >> 4);
    RGB[2].c = RGBC.c;
}

void GTE::op_gpl()
{
}