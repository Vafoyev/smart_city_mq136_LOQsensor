# 🛠 ESP32 Xatoliklarni Tuzatish Qo'llanmasi

Agar siz kodni yozishda (Upload) xatolik ko'rayotgan bo'lsangiz, quyidagilarni tekshiring.

## 1. "Connecting... ____" Xatoligi
Bu eng ko'p uchraydigan muammo. Agar terminalda `Connecting........_____.....` chiqib, keyin `Fatal Error` bersa:

**YECHIM (BOOT TUGMASI):**
1. Kompyuterda **Upload** (➡️) tugmasini bosing.
2. Terminalda `Connecting........` yozuvi chiqqanda, ESP32 platasidagi **BOOT** tugmasini bosib turing.
3. Yozish boshlangandan so'ng (`Writing at ...`), tugmani qo'yib yuboring.

## 2. "COM Port Access Denied"
Agar port band desa:
* Serial Monitorni yoping (Axlat chelak belgisini bosing).
* USB kabelni chiqarib, qayta ulang.

## 3. "Sketch Too Big" (Dastur sig'madi)
Biz `platformio.ini` da `huge_app.csv` ni yoqdik. Bu sizga **3MB** joy beradi. Endi bu xatolik bo'lmasligi kerak.

## 4. Drayverlar
Agar kompyuter ESP32 ni ko'rmayotgan bo'lsa, **CP210x** yoki **CH340** drayverlarini o'rnatish kerak.

---
**Eslatma:** Hozirgi kod 100% to'g'ri va kompilyatsiya bo'ladi. Muammo faqat kompyuter va plata orasidagi ulanishda bo'lishi mumkin.

