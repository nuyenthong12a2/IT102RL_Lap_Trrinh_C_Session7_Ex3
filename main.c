#include <stdio.h>
#include <string.h>

#define MAX_CURRENT 30.0
#define MAX_POWER 6600.0
#define LEAK_CURRENT 0.05
#define TOLERANCE 0.05

struct DeviceUnoptimized {
    char status;
    double measuredPowerW;
    short deviceID;
    float voltageVolts;
    char deviceName[10];
    float currentAmperes;
    int operatingMinutes;
};

struct DeviceFlatOptimized {
    double measuredPowerW;
    float voltageVolts;
    float currentAmperes;
    int operatingMinutes;
    short deviceID;
    char deviceName[10];
    char status;
};

struct ElectricalData {
    double measuredPowerW;
    float voltageVolts;
    float currentAmperes;
};

struct DeviceInfo {
    short deviceID;
    char deviceName[10];
    char status;
};

struct DeviceNested {
    struct ElectricalData electrical;
    int operatingMinutes;
    struct DeviceInfo info;
};

int main() {
    struct DeviceNested device;
    int statusInput;
    int c;
    int len;

    int rawBytes = sizeof(char) + sizeof(double) + sizeof(short) + sizeof(float) + 10 * sizeof(char) + sizeof(float) + sizeof(int);

    printf("=== PHAN TICH BO NHO STRUCT (Memory Padding) ===\n");
    printf("Tong kich thuoc cac truong (khong padding) : %d byte\n", rawBytes);
    printf("sizeof(struct DeviceUnoptimized)           : %d byte (padding %d byte)\n", (int)sizeof(struct DeviceUnoptimized), (int)sizeof(struct DeviceUnoptimized) - rawBytes);
    printf("sizeof(struct DeviceFlatOptimized)         : %d byte (padding %d byte)\n", (int)sizeof(struct DeviceFlatOptimized), (int)sizeof(struct DeviceFlatOptimized) - rawBytes);
    printf("sizeof(struct DeviceNested)                : %d byte (padding %d byte)\n", (int)sizeof(struct DeviceNested), (int)sizeof(struct DeviceNested) - rawBytes);
    printf("Tiet kiem so voi ban chua toi uu           : %d byte\n", (int)sizeof(struct DeviceUnoptimized) - (int)sizeof(struct DeviceNested));

    printf("\n=== NHAP THONG SO THIET BI ===\n");

    printf("Ma thiet bi (so nguyen): ");
    if (scanf("%hd", &device.info.deviceID) != 1) {
        printf("Loi: Ma thiet bi khong hop le!\n");
        return 1;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Ten thiet bi (toi da 9 ky tu): ");
    if (fgets(device.info.deviceName, sizeof(device.info.deviceName), stdin) == NULL) {
        printf("Loi: Khong doc duoc ten thiet bi!\n");
        return 1;
    }
    len = strcspn(device.info.deviceName, "\n");
    if (device.info.deviceName[len] == '\0') {
        while ((c = getchar()) != '\n' && c != EOF);
    }
    device.info.deviceName[len] = '\0';

    printf("Trang thai (0: Tat, 1: Bat): ");
    if (scanf("%d", &statusInput) != 1 || (statusInput != 0 && statusInput != 1)) {
        printf("Loi: Trang thai chi nhan gia tri 0 hoac 1. Dung tinh toan!\n");
        return 1;
    }
    device.info.status = (char)statusInput;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Dien ap U (V): ");
    if (scanf("%f", &device.electrical.voltageVolts) != 1) {
        printf("Loi: Du lieu dien ap khong hop le!\n");
        return 1;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Dong dien I (A): ");
    if (scanf("%f", &device.electrical.currentAmperes) != 1) {
        printf("Loi: Du lieu dong dien khong hop le!\n");
        return 1;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Cong suat do duoc P (W): ");
    if (scanf("%lf", &device.electrical.measuredPowerW) != 1) {
        printf("Loi: Du lieu cong suat khong hop le!\n");
        return 1;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    printf("Thoi gian hoat dong t (phut): ");
    if (scanf("%d", &device.operatingMinutes) != 1) {
        printf("Loi: Thoi gian hoat dong khong hop le!\n");
        return 1;
    }
    while ((c = getchar()) != '\n' && c != EOF);

    if (device.electrical.voltageVolts <= 0 || device.electrical.currentAmperes < 0 ||
        device.electrical.measuredPowerW < 0 || device.operatingMinutes < 0) {
        printf("\n[LOI] Du lieu cam bien rac (U <= 0, I < 0, P < 0 hoac t < 0). Dung tinh toan!\n");
        return 1;
    }

    printf("\n=== KET QUA PHAN TICH AN TOAN ===\n");
    printf("Thiet bi: %s (ID: %d) - %s\n", device.info.deviceName, device.info.deviceID, device.info.status == 1 ? "DANG BAT" : "DANG TAT");

    double pCalc = (double)device.electrical.voltageVolts * device.electrical.currentAmperes;
    double diff = device.electrical.measuredPowerW - pCalc;
    if (diff < 0) {
        diff = -diff;
    }
    printf("Cong suat tinh toan P_calc = U x I = %.2f W\n", pCalc);
    printf("Cong suat do duoc P        = %.2f W\n", device.electrical.measuredPowerW);

    int sensorWarning = 0;
    if (pCalc > 0) {
        if (diff > TOLERANCE * pCalc) {
            sensorWarning = 1;
        }
    } else if (device.electrical.measuredPowerW > 0) {
        sensorWarning = 1;
    }
    if (sensorWarning == 1) {
        printf("[CANH BAO] Cảm biến công suất báo sai lệch\n");
    }

    if (device.electrical.currentAmperes > MAX_CURRENT || device.electrical.measuredPowerW > MAX_POWER) {
        printf("[CANH BAO] QUÁ TẢI NGUY HIỂM\n");
    }

    if (device.info.status == 0 && device.electrical.currentAmperes > LEAK_CURRENT) {
        printf("[CANH BAO] RÒ RỈ ĐIỆN HOẶC TẢI ẨN\n");
    }

    double kwh = device.electrical.measuredPowerW * (device.operatingMinutes / 60.0) / 1000.0;
    printf("Dien nang tieu thu: %.4f kWh\n", kwh);

    return 0;
}