# Bài tập 03: Trade-off giữa struct phẳng và struct lồng nhau trong hệ thống nhúng

## 1. Phân tích Input / Output

**Input** (nhập từ bàn phím):

| Biến | Kiểu | Điều kiện hợp lệ |
|------|------|------------------|
| `deviceID` | `short` | Số nguyên |
| `deviceName` | `char[10]` | Tối đa 9 ký tự (phần thừa bị cắt và xóa khỏi bộ đệm) |
| `status` | `char` (đọc qua biến tạm `int`) | Chỉ nhận 0 (Tắt) hoặc 1 (Bật) |
| `voltageVolts` (U) | `float` | U > 0 |
| `currentAmperes` (I) | `float` | I >= 0 |
| `measuredPowerW` (P) | `double` | P >= 0 |
| `operatingMinutes` (t) | `int` | t >= 0 |

**Output:**
- `sizeof` của 3 cách khai báo struct và số byte padding để so sánh.
- Công suất tính toán `P_calc = U x I` và công suất đo được `P`.
- Các cảnh báo: "Cảm biến công suất báo sai lệch", "QUÁ TẢI NGUY HIỂM", "RÒ RỈ ĐIỆN HOẶC TẢI ẨN".
- Điện năng tiêu thụ `kWh = P x (t / 60.0) / 1000.0`.
- Thông báo lỗi và dừng tính toán nếu dữ liệu rác.

**Giả định đã chọn** (đề chưa nói rõ): các kiểm tra "quá tải công suất" và tính kWh dùng công suất **đo được** từ thiết bị (`measuredPowerW`). `P_calc` chỉ dùng để đối chiếu độ lệch 5%. Khi `P_calc = 0` (I = 0) thì không chia được tỉ lệ phần trăm, nên chương trình cảnh báo sai lệch nếu công suất đo được lớn hơn 0.

## 2. Vì sao struct lãng phí bộ nhớ: Memory Alignment và Padding

Trình biên dịch đặt mỗi member tại địa chỉ chia hết cho kích thước căn chỉnh của kiểu đó (`char`: 1, `short`: 2, `int`/`float`: 4, `double`: 8) và làm tròn kích thước cả struct lên bội số của alignment lớn nhất. Khoảng trống chèn thêm gọi là **padding byte**.

Tổng kích thước thực của các trường: 1 (`status`) + 8 (`double`) + 2 (`short`) + 4 (`float`) + 10 (`char[10]`) + 4 (`float`) + 4 (`int`) = **33 byte**.

### Struct chưa tối ưu (thứ tự tự phát)

| Offset | Trường | Kích thước | Padding chèn thêm |
|--------|--------|-----------|-------------------|
| 0 | `char status` | 1 | **7** (để `double` bắt đầu ở offset 8) |
| 8 | `double measuredPowerW` | 8 | 0 |
| 16 | `short deviceID` | 2 | **2** (để `float` bắt đầu ở offset 20) |
| 20 | `float voltageVolts` | 4 | 0 |
| 24 | `char deviceName[10]` | 10 | **2** (để `float` bắt đầu ở offset 36) |
| 36 | `float currentAmperes` | 4 | 0 |
| 40 | `int operatingMinutes` | 4 | **4** (làm tròn tổng thành bội của 8) |
| | **Tổng** | **48** | **15** |

## 3. Hai giải pháp tái cấu trúc

### Giải pháp 1: Struct phẳng, sắp xếp giảm dần theo kích thước kiểu

```c
struct DeviceFlatOptimized {
    double measuredPowerW;   // 8
    float voltageVolts;      // 4
    float currentAmperes;    // 4
    int operatingMinutes;    // 4
    short deviceID;          // 2
    char deviceName[10];     // 10
    char status;             // 1
};
```

Các trường lớn nằm trước nên mỗi trường tự rơi đúng vào địa chỉ chia hết cho alignment của nó, không cần padding ở giữa. Chỉ còn **7 byte padding ở cuối** để làm tròn 33 lên 40.

### Giải pháp 2: Struct lồng nhau, nhóm theo chức năng

```c
struct ElectricalData { double measuredPowerW; float voltageVolts; float currentAmperes; };
struct DeviceInfo     { short deviceID; char deviceName[10]; char status; };
struct DeviceNested   { struct ElectricalData electrical; int operatingMinutes; struct DeviceInfo info; };
```

Nhóm điện (16 byte, alignment 8) và nhóm thông tin (14 byte, alignment 2) được đặt sao cho trường có alignment lớn đứng trước. `operatingMinutes` nằm ở offset 16 và `info` bắt đầu ở offset 20, nên cũng chỉ còn 7 byte padding ở cuối.

### Kết quả `sizeof` đo thực tế (GCC, x86-64)

```
Tong kich thuoc cac truong (khong padding) : 33 byte
sizeof(struct DeviceUnoptimized)           : 48 byte (padding 15 byte)
sizeof(struct DeviceFlatOptimized)         : 40 byte (padding 7 byte)
sizeof(struct DeviceNested)                : 40 byte (padding 7 byte)
Tiet kiem so voi ban chua toi uu           : 8 byte
```

Cả hai giải pháp đều giảm từ 48 xuống 40 byte (tiết kiệm 8 byte, khoảng 16,7% mỗi thiết bị). Con số 40 là mức thấp nhất có thể khi còn dùng `double`, vì tổng 33 byte phải làm tròn lên bội của 8. Số liệu này đúng với GCC trên x86-64; kiến trúc khác có thể khác, nên luôn kiểm tra bằng `sizeof`.

## 4. So sánh hai giải pháp

| Tiêu chí | Giải pháp 1: Struct phẳng đã sắp xếp | Giải pháp 2: Struct lồng nhau |
|----------|--------------------------------------|-------------------------------|
| Dung lượng (`sizeof`) | 40 byte | 40 byte nếu sắp xếp đúng; nếu nhóm tùy ý thì mỗi struct con có thể tự sinh padding riêng (lồng nhau mà không tính toán có thể lớn hơn) |
| Độ phức tạp truy xuất | Đơn giản: `device.currentAmperes` | Dài hơn: `device.electrical.currentAmperes`. Offset được tính lúc biên dịch nên không tốn thêm thời gian chạy |
| Tính dễ bảo trì | Struct dài dần khi thêm trường, các nhóm chức năng bị trộn lẫn; sắp xếp theo kích thước phá vỡ thứ tự logic | Mỗi nhóm có ý nghĩa rõ ràng (điện, thông tin), dễ đọc |
| Khả năng mở rộng | Thêm trường mới có thể làm hỏng thứ tự tối ưu, phải sắp xếp lại toàn bộ | Thêm trường vào đúng nhóm, chỉ cần kiểm tra lại nhóm đó; có thể tái dùng `ElectricalData` cho loại thiết bị khác |
| Rủi ro | Dễ quên nguyên tắc sắp xếp khi sửa | Cần kiểm tra padding của từng struct con và thứ tự ghép |

## 5. Lựa chọn cho IoT Gateway

**Chọn Giải pháp 2 (struct lồng nhau, với thứ tự được sắp xếp có chủ ý trong từng nhóm).**

Lý do:
1. **Hiệu quả bộ nhớ không thua kém.** Cả hai đều đạt 40 byte, nên giải pháp lồng nhau không phải trả giá bằng RAM.
2. **Không tốn chi phí chạy.** Truy xuất `a.b.c` được biên dịch thành một offset cố định, tốc độ như truy xuất phẳng.
3. **Dễ bảo trì khi hệ thống mở rộng.** Gateway quản lý nhiều loại thiết bị, có thể tái dùng nhóm `ElectricalData` ở mọi loại thiết bị và chỉ cần tối ưu padding một lần cho nhóm đó.

Điều kiện đi kèm: sau mỗi lần thay đổi struct phải in lại `sizeof` (như chương trình đã làm) để chắc chắn không phát sinh padding mới. Nếu bộ nhớ quá hạn hẹp và RAM là ưu tiên số một, có thể đổi `measuredPowerW` từ `double` sang `float` để toàn bộ struct chỉ cần alignment 4 (tổng 29 byte, làm tròn thành 32 byte).

## 6. Thiết kế chi tiết (mã giả) cho giải pháp đã chọn

```
BAT DAU
  In sizeof của 3 struct và padding để so sánh
  Nhập deviceID, deviceName (fgets, xóa '\n', xóa phần thừa trong bộ đệm),
       status, U, I, P, t
  NEU status khác 0 và khác 1  -> báo lỗi, dừng
  NEU U <= 0 HOẶC I < 0 HOẶC P < 0 HOẶC t < 0 -> báo lỗi, dừng
  P_calc = U * I
  độ lệch = |P - P_calc|
  NEU P_calc > 0 VÀ độ lệch > 5% * P_calc  -> cảnh báo "Cảm biến công suất báo sai lệch"
  NEU P_calc = 0 VÀ P > 0                  -> cảnh báo "Cảm biến công suất báo sai lệch"
  NEU I > 30.0 HOẶC P > 6600.0             -> cảnh báo "QUÁ TẢI NGUY HIỂM"
  NEU status = 0 VÀ I > 0.05               -> cảnh báo "RÒ RỈ ĐIỆN HOẶC TẢI ẨN"
  kWh = P * (t / 60.0) / 1000.0            (chia cho 60.0 để tránh chia số nguyên)
  In kWh
KET THUC
```

## 7. Kiểm thử

| Trường hợp | Đầu vào (U, I, P, t, status) | Kết quả |
|------------|------------------------------|---------|
| Bình thường | 220, 20, 4400, 90, 1 | Không cảnh báo; 6.6000 kWh |
| Cảm biến lệch 11% | 220, 20, 4900, 60, 1 | Cảnh báo sai lệch; 4.9000 kWh |
| Quá tải dòng điện | 220, 35, 7700, 30, 1 | QUÁ TẢI NGUY HIỂM; 3.8500 kWh |
| Quá tải công suất (I = 28 A) | 250, 28, 7000, 60, 1 | QUÁ TẢI NGUY HIỂM; 7.0000 kWh |
| Rò rỉ điện | 220, 0.2, 44, 120, 0 | RÒ RỈ ĐIỆN HOẶC TẢI ẨN; 0.0880 kWh |
| Tắt hoàn toàn | 220, 0, 0, 120, 0 | Không cảnh báo; 0.0000 kWh |
| U = 0 | 0, 5, 10, 10, 1 | Báo lỗi dữ liệu rác, dừng |
| status = 2 | 220, 5, 1100, 10, 2 | Báo lỗi trạng thái, dừng |
| t âm | 220, 5, 1100, -5, 1 | Báo lỗi dữ liệu rác, dừng |

## 8. Biên dịch và chạy

```
gcc -Wall -Wextra -std=c11 main.c -o main
./main
```

Commit khi nộp: `refactor(struct): optimize memory layout for smart device struct`