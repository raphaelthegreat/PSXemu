#include "gte.h"
#include <cpu/util.h>
#include <glm/gtx/string_cast.hpp>

#pragma optimize("", off)

GTE::GTE(CPU* _cpu)
{
    cpu = _cpu;

    register_opcodes();
}

uint32_t GTE::read_control(uint32_t reg)
{
    return control.reg[reg];
}

uint32_t GTE::read_data(uint32_t reg)
{
    return data.reg[reg];
}

void GTE::interpolate(int32_t mac1, int32_t mac2, int32_t mac3)
{
    auto& mc1 = data.macvec[0];
    auto& mc2 = data.macvec[1];
    auto& mc3 = data.macvec[2];
    auto& fc = control.far_color;
    auto lm = command.lm;
    auto sf = command.sf;
    
    mc1 = (int)(set_mac_flag(1, ((int64_t)fc.r << 12) - mac1) >> sf * 12);
    mc2 = (int)(set_mac_flag(2, ((int64_t)fc.g << 12) - mac2) >> sf * 12);
    mc3 = (int)(set_mac_flag(3, ((int64_t)fc.b << 12) - mac3) >> sf * 12);

    data.ir1 = set_ir_flag(1, mc1, false);
    data.ir2 = set_ir_flag(2, mc2, false);
    data.ir3 = set_ir_flag(3, mc3, false);

    mc1 = (int)(set_mac_flag(1, ((int64_t)data.ir1 * data.ir0) + mac1) >> sf * 12);
    mc2 = (int)(set_mac_flag(2, ((int64_t)data.ir2 * data.ir0) + mac2) >> sf * 12);
    mc3 = (int)(set_mac_flag(3, ((int64_t)data.ir3 * data.ir0) + mac3) >> sf * 12);

    data.ir1 = set_ir_flag(1, mc1, lm);
    data.ir2 = set_ir_flag(2, mc2, lm);
    data.ir3 = set_ir_flag(3, mc3, lm);

    std::cout << data.ir0 << " " << data.ir1 << " " << data.ir2 << " " << data.ir3 << "\n";
}

void GTE::execute(Instr instr)
{
    /* Convert to GTE command format. */
    command.raw = instr.value;

    auto rot = glm::to_string(control.rotation.matrix);
    auto tr = glm::to_string(control.translation);
    auto llm = glm::to_string(control.light_color.matrix);
    auto bg = glm::to_string(control.bg_color);
    auto sz = glm::to_string(glm::ivec4(data.sz0, data.sz1, data.sz2, data.sz3));

    auto& handler = lookup[command.opcode];
    if (handler != nullptr)
        handler();
    else
        __debugbreak();
}

void GTE::write_data(uint32_t reg, uint32_t v)
{
    data.reg[reg] = v;
}

void GTE::write_control(uint32_t reg, uint32_t v)
{
    control.reg[reg] = v;
}

inline int64_t GTE::set_mac_flag(int i, int64_t value)
{
    auto& flag = control.flag.raw;
    
    if (value < -0x80000000000LL)
        flag = set_bit(flag, 28 - i, true);    

    if (value > 0x7FFFFFFFFFFLL)
        flag = set_bit(flag, 31 - i, true);    

    return (value << 20) >> 20;
}

inline int32_t GTE::set_mac0_flag(int64_t value)
{
    auto& flag = control.flag;

    if (value < -0x80000000LL)
        flag.mac0_negative = true;   
    else if (value > 0x7FFFFFFF)
        flag.mac0_positive = true;    

    return (int32_t)value;
}

inline uint16_t GTE::set_sz3_flag(int64_t value)
{
    auto& flag = control.flag;
    
    if (value < 0) {
        flag.sz3_otz_saturated = true;
        return 0;
    }
    
    if (value > 0xFFFF) {
        flag.sz3_otz_saturated = true;
        return 0xFFFF;
    }

    return (uint16_t)value;
}

inline int16_t GTE::set_sxy_flag(int i, int32_t value)
{
    auto& flag = control.flag.raw;
    
    if (value < -0x400) {
        flag |= (uint32_t)(0x4000 >> (i - 1));
        return -0x400;
    }

    if (value > 0x3FF) {
        flag |= (uint32_t)(0x4000 >> (i - 1));
        return 0x3FF;
    }

    return (int16_t)value;
}

inline int16_t GTE::set_ir0_flag(int64_t value)
{
    auto& flag = control.flag;
    
    if (value < 0) {
        flag.ir0_saturated = true;
        return 0;
    }

    if (value > 0x1000) {
        flag.ir0_saturated = true;
        return 0x1000;
    }

    return (int16_t)value;
}

inline int16_t GTE::set_ir_flag(int i, int32_t value, bool lm)
{
    auto& flag = control.flag.raw;
    
    if (lm && value < 0) {
        flag |= (uint32_t)(0x1000000 >> (i - 1));
        return 0;
    }

    if (!lm && (value < -0x8000)) {
        flag |= (uint32_t)(0x1000000 >> (i - 1));
        return -0x8000;
    }

    if (value > 0x7FFF) {
        flag |= (uint32_t)(0x1000000 >> (i - 1));
        return 0x7FFF;
    }

    return (int16_t)value;
}

inline uint8_t GTE::set_rgb(int i, int value) 
{
    auto& flag = control.flag.raw;

    if (value < 0) {
        flag |= (uint32_t)0x200000 >> (i - 1);
        return 0;
    }

    if (value > 0xFF) {
        flag |= (uint32_t)0x200000 >> (i - 1);
        return 0xFF;
    }

    return (uint8_t)value;
}

void GTE::op_rtps()
{
}

void GTE::op_rtpt()
{
    auto& mac1 = data.macvec[0];
    auto& mac2 = data.macvec[1];
    auto& mac3 = data.macvec[2];
    auto& sxy = data.sxy;
    auto& v = data.v;    
    auto& tr = control.translation;
    auto& rt = control.rotation;
    auto sf = command.sf;

    for (int i = 0; i < 3; i++) {
        mac1 = (int)set_mac_flag(1, (tr.x * 0x1000 + rt[0][0] * v[i].x + rt[0][1] * v[i].y + rt[0][2] * v[i].z) >> (sf * 12));
        mac2 = (int)set_mac_flag(2, (tr.y * 0x1000 + rt[1][0] * v[i].x + rt[1][1] * v[i].y + rt[1][2] * v[i].z) >> (sf * 12));
        mac3 = (int)set_mac_flag(3, (tr.z * 0x1000 + rt[2][0] * v[i].x + rt[1][2] * v[i].y + rt[2][2] * v[i].z) >> (sf * 12));

        data.ir1 = set_ir_flag(1, mac1, false);
        data.ir2 = set_ir_flag(2, mac2, false);
        data.ir3 = set_ir_flag(3, mac3, false);

        data.sz0 = data.sz1;
        data.sz1 = data.sz2;
        data.sz2 = data.sz3;
        data.sz3 = set_sz3_flag(mac3 >> ((1 - sf) * 12));

        sxy[0] = sxy[1];
        sxy[1] = sxy[2];

        int64_t result;
        int64_t div = 0;
        if (data.sz3 == 0) {
            result = 0x1FFFF;
        }
        else {
            div = (((int64_t)control.proj_dist * 0x20000 / data.sz3) + 1) / 2;

            if (div > 0x1FFFF) {
                result = 0x1FFFF;
                control.flag.divide_overflow = true;
            }
            else {
                result = div;
            }
        }

        data.mac0 = (int)(result * data.ir1 + control.screen_offset.x);
        sxy[2].x = set_sxy_flag(2, data.mac0 / 0x10000);
        
        data.mac0 = (int)(result * data.ir2 + control.screen_offset.y);
        sxy[2].y = set_sxy_flag(2, data.mac0 / 0x10000);
        
        data.mac0 = (int)(result * control.dqa + control.dqb);
        data.ir0 = set_ir0_flag(data.mac0 / 0x1000);
    }
}

void GTE::op_mvmva()
{
}

void GTE::op_dcpl()
{
}

void GTE::op_dpcs()
{
}

void GTE::op_intpl()
{
}

void GTE::op_sqr()
{
}

void GTE::op_ncs()
{
}

void GTE::op_ncds()
{
    auto& mac1 = data.macvec[0];
    auto& mac2 = data.macvec[1];
    auto& mac3 = data.macvec[2];
    auto& v = data.v;
    auto& llm = control.light_src;
    auto& lcm = control.light_color;
    auto& rgbc = data.color;
    auto& rgb = data.rgb;
    auto& bc = control.bg_color;
    auto sf = command.sf;
    auto lm = command.lm;

    mac1 = (int32_t)(set_mac_flag(1, (int64_t)llm[0][0] * v[0].x + llm[0][1] * v[0].y + llm[0][2] * v[0].z) >> sf * 12);
    mac2 = (int32_t)(set_mac_flag(2, (int64_t)llm[1][0] * v[0].x + llm[1][1] * v[0].y + llm[1][2] * v[0].z) >> sf * 12);
    mac3 = (int32_t)(set_mac_flag(3, (int64_t)llm[2][0] * v[0].x + llm[2][1] * v[0].y + llm[2][2] * v[0].z) >> sf * 12);

    data.ir1 = set_ir_flag(1, mac1, lm);
    data.ir2 = set_ir_flag(2, mac2, lm);
    data.ir3 = set_ir_flag(3, mac3, lm);

    /* WARNING: Each multiplication can trigger mac flags so the check is needed on each op! */
    mac1 = (int)(set_mac_flag(1, set_mac_flag(1, set_mac_flag(1, (int64_t)bc.r * 0x1000 + lcm[0][0] * data.ir1) + (int64_t)lcm[0][1] * data.ir2) + (int64_t)lcm[0][2] * data.ir3) >> sf * 12);
    mac2 = (int)(set_mac_flag(2, set_mac_flag(2, set_mac_flag(2, (int64_t)bc.g * 0x1000 + lcm[1][0] * data.ir1) + (int64_t)lcm[1][1] * data.ir2) + (int64_t)lcm[1][2] * data.ir3) >> sf * 12);
    mac3 = (int)(set_mac_flag(3, set_mac_flag(3, set_mac_flag(3, (int64_t)bc.b * 0x1000 + lcm[2][0] * data.ir1) + (int64_t)lcm[2][1] * data.ir2) + (int64_t)lcm[2][2] * data.ir3) >> sf * 12);

    data.ir1 = set_ir_flag(1, mac1, lm);
    data.ir2 = set_ir_flag(2, mac2, lm);
    data.ir3 = set_ir_flag(3, mac3, lm);

    mac1 = (int)set_mac_flag(1, ((int64_t)rgbc.r * data.ir1) << 4);
    mac2 = (int)set_mac_flag(2, ((int64_t)rgbc.g * data.ir2) << 4);
    mac3 = (int)set_mac_flag(3, ((int64_t)rgbc.b * data.ir3) << 4);
    
    interpolate(mac1, mac2, mac3);

    rgb[0] = rgb[1];
    rgb[1] = rgb[2];

    rgb[2].r = set_rgb(1, mac1 >> 4);
    rgb[2].g = set_rgb(2, mac2 >> 4);
    rgb[2].b = set_rgb(3, mac3 >> 4);
    rgb[2].c = rgbc.c;
}

void GTE::op_nccs()
{

}

void GTE::op_cdp()
{

}

void GTE::op_cc()
{

}

void GTE::op_nclip()
{
    auto& mac0 = data.mac0;
    auto& sxy = data.sxy;

    mac0 = set_mac0_flag((int64_t)sxy[0].x * sxy[1].y + sxy[1].x * sxy[2].y + sxy[2].x * sxy[0].y - sxy[0].x * sxy[2].y - sxy[1].x * sxy[0].y - sxy[2].x * sxy[1].y);
}

void GTE::op_avsz3()
{
    //std::cout << "GTE AVSZ3 command\n";

    /* Compute the average of the z values. */
    auto& mac0 = data.mac0;
    auto& otz = data.otz;
    auto& zsf3 = control.zsf3;
    
    int64_t avsz3 = (int64_t)zsf3 * (data.sz1 + data.sz2 + data.sz3);
    mac0 = set_mac0_flag(avsz3);
    otz = set_sz3_flag(avsz3 >> 12);
}

void GTE::op_avsz4()
{

}

void GTE::op_op()
{

}

void GTE::op_gpf()
{

}

void GTE::op_gpl()
{

}
