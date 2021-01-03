#include <stdio.h>
#include <ftd2xx.h>
#include <log.h>
#include <stdbool.h>
#include <string.h>
#include <rsp_types.h>
#include <rsp_vector_instructions.h>
#include <rsp.h>

static unsigned int num_devices;
static FT_DEVICE_LIST_INFO_NODE* device_info;
static FT_HANDLE handle;

unsigned int bytes_written;
unsigned int bytes_read;

static rsp_t rsp;


typedef union flag_result {
    struct {
        half vcc;
        half vco;
        half vce;
        half padding;
    };
    dword packed;
} flag_result_t;
static_assert(sizeof(flag_result_t) == sizeof(dword), "flag_result_t should be 64 bits");

typedef struct rsp_testable_instruction {
    rspinstr_handler_t handler;
    int funct;
    const char* name;
} rsp_testable_instruction_t;
#define INSTR(_handler, _funct, _name) { .handler = _handler, .funct = _funct, .name = _name}


rsp_testable_instruction_t instrs[] = {
        INSTR(rsp_vec_vabs, FUNCT_RSP_VEC_VABS, "vabs"),
        INSTR(rsp_vec_vadd, FUNCT_RSP_VEC_VADD, "vadd"),
        INSTR(rsp_vec_vaddc, FUNCT_RSP_VEC_VADDC, "vaddc"),
        INSTR(rsp_vec_vand, FUNCT_RSP_VEC_VAND, "vand"),
        INSTR(rsp_vec_vch, FUNCT_RSP_VEC_VCH, "vch"),
        INSTR(rsp_vec_vcl, FUNCT_RSP_VEC_VCL, "vcl"),
        INSTR(rsp_vec_vcr, FUNCT_RSP_VEC_VCR, "vcr"),
        INSTR(rsp_vec_veq, FUNCT_RSP_VEC_VEQ, "veq"),
        INSTR(rsp_vec_vge, FUNCT_RSP_VEC_VGE, "vge"),
        INSTR(rsp_vec_vlt, FUNCT_RSP_VEC_VLT, "vlt"),
        INSTR(rsp_vec_vmacf, FUNCT_RSP_VEC_VMACF, "vmacf"),
        //INSTR(rsp_vec_vmacq, FUNCT_RSP_VEC_VMACQ, "vmacq"),
        INSTR(rsp_vec_vmacu, FUNCT_RSP_VEC_VMACU, "vmacu"),
        INSTR(rsp_vec_vmadh, FUNCT_RSP_VEC_VMADH, "vmadh"),
        INSTR(rsp_vec_vmadl, FUNCT_RSP_VEC_VMADL, "vmadl"),
        INSTR(rsp_vec_vmadm, FUNCT_RSP_VEC_VMADM, "vmadm"),
        INSTR(rsp_vec_vmadn, FUNCT_RSP_VEC_VMADN, "vmadn"),
        INSTR(rsp_vec_vmov, FUNCT_RSP_VEC_VMOV, "vmov"),
        INSTR(rsp_vec_vmrg, FUNCT_RSP_VEC_VMRG, "vmrg"),
        INSTR(rsp_vec_vmudh, FUNCT_RSP_VEC_VMUDH, "vmudh"),
        INSTR(rsp_vec_vmudl, FUNCT_RSP_VEC_VMUDL, "vmudl"),
        INSTR(rsp_vec_vmudm, FUNCT_RSP_VEC_VMUDM, "vmudm"),
        INSTR(rsp_vec_vmudn, FUNCT_RSP_VEC_VMUDN, "vmudn"),
        INSTR(rsp_vec_vmulf, FUNCT_RSP_VEC_VMULF, "vmulf"),
        //INSTR(rsp_vec_vmulq, FUNCT_RSP_VEC_VMULQ, "vmulq"),
        INSTR(rsp_vec_vmulu, FUNCT_RSP_VEC_VMULU, "vmulu"),
        INSTR(rsp_vec_vnand, FUNCT_RSP_VEC_VNAND, "vnand"),
        INSTR(rsp_vec_vne, FUNCT_RSP_VEC_VNE, "vne"),
        //INSTR(rsp_vec_vnop, FUNCT_RSP_VEC_VNOP, "vnop"),
        INSTR(rsp_vec_vnor, FUNCT_RSP_VEC_VNOR, "vnor"),
        INSTR(rsp_vec_vnxor, FUNCT_RSP_VEC_VNXOR, "vnxor"),
        INSTR(rsp_vec_vor, FUNCT_RSP_VEC_VOR, "vor"),
        INSTR(rsp_vec_vrcp, FUNCT_RSP_VEC_VRCP, "vrcp"),
        INSTR(rsp_vec_vrcph_vrsqh, FUNCT_RSP_VEC_VRCPH, "vrcph_vrsqh"),
        INSTR(rsp_vec_vrcpl, FUNCT_RSP_VEC_VRCPL, "vrcpl"),
        //INSTR(rsp_vec_vrndn, FUNCT_RSP_VEC_VRNDN, "vrndn"),
        //INSTR(rsp_vec_vrndp, FUNCT_RSP_VEC_VRNDP, "vrndp"),
        INSTR(rsp_vec_vrsq, FUNCT_RSP_VEC_VRSQ, "vrsq"),
        INSTR(rsp_vec_vrsql, FUNCT_RSP_VEC_VRSQL, "vrsql"),
        INSTR(rsp_vec_vsar, FUNCT_RSP_VEC_VSAR, "vsar"),
        INSTR(rsp_vec_vsub, FUNCT_RSP_VEC_VSUB, "vsub"),
        INSTR(rsp_vec_vsubc, FUNCT_RSP_VEC_VSUBC, "vsubc"),
        INSTR(rsp_vec_vxor, FUNCT_RSP_VEC_VXOR, "vxor"),
};

void assert_ftcommand(FT_STATUS result, const char* message) {
    if (result != FT_OK) {
        logdie("%s", message);
    }
}

bool is_everdrive(unsigned int device) {
    return (strcmp(device_info[device].Description, "FT245R USB FIFO") == 0 && device_info[device].ID == 0x04036001);
}

void send_vreg(vu_reg_t* reg) {
    vu_reg_t swapped;
    memcpy(&swapped, reg, sizeof(vu_reg_t));

    for (int i = 0; i < 8; i++) {
        swapped.elements[i] = htobe16(swapped.elements[i]);
    }

    assert_ftcommand(FT_Write(handle, &swapped, sizeof(vu_reg_t), &bytes_written), "Unable to write vu reg!");
}

void send_instruction(word instr) {
    word swapped = htobe32(instr);
    assert_ftcommand(FT_Write(handle, &swapped, sizeof(word), &bytes_written), "Unable to write instruction!");
}

void recv_vreg(vu_reg_t* reg) {
    assert_ftcommand(FT_Read(handle, reg, sizeof(vu_reg_t), &bytes_read), "Unable to read vu reg!");
    for (int i = 0; i < 8; i++) {
        reg->elements[i] = be16toh(reg->elements[i]);
    }
}

void recv_flag_result(flag_result_t* result) {
    assert_ftcommand(FT_Read(handle, result, sizeof(flag_result_t), &bytes_read), "Unable to read flag_result!");
    result->vcc = be16toh(result->vcc);
    result->vco = be16toh(result->vco);
    result->vce = be16toh(result->vce);
}

void init_everdrive(unsigned int device) {
    // initialize
    assert_ftcommand(FT_Open(device, &handle), "Unable to open device!");
    assert_ftcommand(FT_ResetDevice(handle), "Unable to reset device!");
    assert_ftcommand(FT_SetTimeouts(handle, 5000, 5000), "Unable to set timeouts!");
    assert_ftcommand(FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX), "Unable to purge USB buffer contents!");
}

void print_vureg_ln(vu_reg_t* reg) {
    for (int i = 0; i < 8; i++) {
        printf("%04X ", reg->elements[i]);
    }
    printf("\n");
}

bool print_vureg_comparing_ln(vu_reg_t* reg, vu_reg_t* compare) {
    bool any_bad = false;
    for (int i = 0; i < 8; i++) {
        if (compare->elements[i] != reg->elements[i]) {
            any_bad = true;
            printf(COLOR_RED);
        }
        printf("%04X ", reg->elements[i]);
        if (compare->elements[i] != reg->elements[i]) {
            printf(COLOR_END);
        }
    }
    printf("\n");
    return any_bad;
}

void check_emu(rspinstr_handler_t handler, vu_reg_t vs, vu_reg_t vt, int e, vu_reg_t* res, vu_reg_t* acc_h, vu_reg_t* acc_m, vu_reg_t* acc_l, flag_result_t* flag_result) {
    mips_instruction_t instruction;
    instruction.raw = 0;

    instruction.cp2_vec.vs = 1;
    instruction.cp2_vec.vt = 2;
    instruction.cp2_vec.vd = 3;
    instruction.cp2_vec.e = e;


    for (int i = 0; i < 8; i++) {
        rsp.vu_regs[1].elements[7 - i] = vs.elements[i];
        rsp.vu_regs[2].elements[7 - i] = vt.elements[i];
    }

    handler(&rsp, instruction);

    flag_result->vcc = rsp_get_vcc(&rsp);
    flag_result->vco = rsp_get_vco(&rsp);
    flag_result->vce = rsp_get_vce(&rsp);

    for (int i = 0; i < 8; i++) {
        res->elements[i] = rsp.vu_regs[3].elements[7 - i];
        acc_h->elements[i] = rsp.acc.h.elements[7 - i];
        acc_m->elements[i] = rsp.acc.m.elements[7 - i];
        acc_l->elements[i] = rsp.acc.l.elements[7 - i];
    }
}

half rand_half() {
    return rand() & 0xFFFF;
}

word vec_instr(int funct) {
    int vs = 1;
    int vt = 2;
    int vd = 3;

    if (funct != (funct & 0b111111)) {
        logfatal("FUNCT would overflow!\n");
    }

    return OPC_CP2 << 26 | 1 << 25 | vt << 16 | vs << 11 | vd << 6 | funct;
}

void run_test(vu_reg_t arg1, vu_reg_t arg2, rsp_testable_instruction_t* instr) {
    send_instruction(vec_instr(instr->funct));
    send_vreg(&arg1);
    send_vreg(&arg2);

    printf("Testing %s\n", instr->name);
    printf("args:\n");
    print_vureg_ln(&arg1);
    print_vureg_ln(&arg2);
    printf("Result:\n");

    bool any_bad = false;
    for (int element = 0; element < 16; element++) {
        vu_reg_t res;
        recv_vreg(&res);

        vu_reg_t acc_h;
        recv_vreg(&acc_h);

        vu_reg_t acc_m;
        recv_vreg(&acc_m);

        vu_reg_t acc_l;
        recv_vreg(&acc_l);

        flag_result_t flag_result;
        recv_flag_result(&flag_result);

        vu_reg_t emu_res;
        vu_reg_t emu_acc_h;
        vu_reg_t emu_acc_m;
        vu_reg_t emu_acc_l;
        flag_result_t emu_flag_result;
        check_emu(instr->handler, arg1, arg2, element, &emu_res, &emu_acc_h, &emu_acc_m, &emu_acc_l, &emu_flag_result);


        printf("element %02d, n64 res: ", element);
        print_vureg_ln(&res);
        printf("element %02d, emu res: ", element);
        any_bad |= print_vureg_comparing_ln(&emu_res, &res);

        printf("element %02d, n64 acc_h: ", element);
        print_vureg_ln(&acc_h);
        printf("element %02d, emu acc_h: ", element);
        any_bad |= print_vureg_comparing_ln(&emu_acc_h, &acc_h);

        printf("element %02d, n64 acc_m: ", element);
        print_vureg_ln(&acc_m);
        printf("element %02d, emu acc_m: ", element);
        any_bad |= print_vureg_comparing_ln(&emu_acc_m, &acc_m);

        printf("element %02d, n64 acc_l: ", element);
        print_vureg_ln(&acc_l);
        printf("element %02d, emu acc_l: ", element);
        any_bad |= print_vureg_comparing_ln(&emu_acc_l, &acc_l);

        const char* color_start = (flag_result.vcc != emu_flag_result.vcc) ? COLOR_RED : "";
        const char* color_end   = (flag_result.vcc != emu_flag_result.vcc) ? COLOR_END : "";
        printf("element %02d, n64 vcc: 0x%04X emu vcc: %s0x%04X%s\n", element, flag_result.vcc, color_start, emu_flag_result.vcc, color_end);
        any_bad |= (flag_result.vcc != emu_flag_result.vcc);

        color_start = (flag_result.vco != emu_flag_result.vco) ? COLOR_RED : "";
        color_end   = (flag_result.vco != emu_flag_result.vco) ? COLOR_END : "";
        printf("element %02d, n64 vco: 0x%04X emu vco: %s0x%04X%s\n", element, flag_result.vco, color_start, emu_flag_result.vco, color_end);
        any_bad |= (flag_result.vco != emu_flag_result.vco);

        color_start = (flag_result.vce != emu_flag_result.vce) ? COLOR_RED : "";
        color_end   = (flag_result.vce != emu_flag_result.vce) ? COLOR_END : "";
        printf("element %02d, n64 vce: 0x%04X emu vce: %s0x%04X%s\n", element, flag_result.vce, color_start, emu_flag_result.vce, color_end);
        any_bad |= (flag_result.vce != emu_flag_result.vce);
        printf("\n");
    }

    if (any_bad) {
        logfatal("Data mismatch!");
    }
}

void run_random_test(rsp_testable_instruction_t* instr) {
    vu_reg_t arg1;
    vu_reg_t arg2;

    arg1.elements[0] = rand_half();
    arg1.elements[1] = rand_half();
    arg1.elements[2] = rand_half();
    arg1.elements[3] = rand_half();
    arg1.elements[4] = rand_half();
    arg1.elements[5] = rand_half();
    arg1.elements[6] = rand_half();
    arg1.elements[7] = rand_half();

    arg2.elements[0] = rand_half();
    arg2.elements[1] = rand_half();
    arg2.elements[2] = rand_half();
    arg2.elements[3] = rand_half();
    arg2.elements[4] = rand_half();
    arg2.elements[5] = rand_half();
    arg2.elements[6] = rand_half();
    arg2.elements[7] = rand_half();

    run_test(arg1, arg2, instr);

}

int main(int argc, char** argv) {
    memset(&rsp, 0, sizeof(rsp_t));
    srand(time(NULL));
    if (FT_CreateDeviceInfoList(&num_devices) != FT_OK) {
        logdie("Unable to enumerate num_devices. Try again?");
    }

    if (num_devices == 0) {
        logdie("No devices found. Is your EverDrive plugged in?");
    }

    logalways("Found %d device%s.", num_devices, num_devices == 1 ? "" : "s");

    device_info = malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * num_devices);
    FT_GetDeviceInfoList(device_info, &num_devices);

    int everdrive_index = -1;
    for (int i = 0; i < num_devices && everdrive_index < 0; i++) {
        if (is_everdrive(i)) {
            everdrive_index = i;
        }
    }

    if (everdrive_index < 0) {
        logdie("Did not find an everdrive!\n");
    }

    init_everdrive(everdrive_index);

    while (true) {
        for (int instr = 0; instr < (sizeof(instrs) / sizeof(rsp_testable_instruction_t)); instr++) {
            printf("Fuzzing %s 1000 times...\n", instrs[instr].name);
            for (int test = 0; test < 1000; test++) {
                run_random_test(&instrs[instr]);
            }
        }
    }
}