# 📡 Smart City Sensor - API & JSON Documentation

ESP32 qurilmasi serverga **HTTP POST** so'rovi orqali ma'lumotlarni **JSON** formatida yuboradi.

## 1. JSON Tuzilishi (Payload)

Qurilma har doim quyidagi formatda ma'lumot yuboradi:

```json
{
  "alarm": false,
  "fire": false,
  "quake": false,
  "lpg": 0.45,
  "gas": 0.32,
  "co": 0.12,
  "temp": 24.5
}
```

### Maydonlar (Fields) tavsifi:

| Maydon | Turi | Tavsif |
| :--- | :---: | :--- |
| `alarm` | `boolean` | Umumiy xavf holati (`true` = Xavf bor, `false` = Tinch) |
| `fire` | `boolean` | Yong'in sensori (`true` = Olov aniqlandi) |
| `quake` | `boolean` | Zilzila/Silkinish (`true` = 0.6G dan yuqori silkinish) |
| `lpg` | `float` | MQ-6 (Propan/Butan) datchigi kuchlanishi (0.00v - 3.30v) |
| `gas` | `float` | MQ-9 (Yonuvchi gazlar) datchigi kuchlanishi (0.00v - 3.30v) |
| `co` | `float` | MQ-7 (Is gazi) datchigi kuchlanishi (0.00v - 3.30v) |
| `temp` | `float` | MPU6050 ichidagi harorat sensori (°C) |

---

## 2. Serverda Qabul Qilish (Misollar)

Agar siz o'z serveringizni yozayotgan bo'lsangiz, quyidagi kodlardan foydalanishingiz mumkin.

### PHP (backend.php)
```php
<?php
// JSON oqimini olish
$json = file_get_contents('php://input');
$data = json_decode($json, true);

if ($data) {
    $alarm = $data['alarm']; // true yoki false
    $temp  = $data['temp'];  // 24.5

    // Ma'lumotlar bazasiga yozish yoki Telegramga yuborish...
    file_put_contents("sensor_log.txt", "Temp: " . $temp . " - Alarm: " . ($alarm ? 'YES':'NO') . "\n", FILE_APPEND);
    
    echo "Data Received";
} else {
    echo "No Data";
}
?>
```

### Node.js (Express)
```javascript
app.post('/api/data', (req, res) => {
    const { alarm, fire, temp } = req.body;
    
    console.log(`Harorat: ${temp}, Xavf: ${alarm}`);
    
    res.send("OK");
});
```

## 3. Qurilma Sozlamalari

`src/main.cpp` faylida serveringiz manzilini kiritishni unutmang:

```cpp
const char* SERVER_URL = "http://sizning-serveringiz.com/api/data"; // <-- O'zgartiring!
```

---
**Eslatma:** Ma'lumotlar xavfsiz holatda har 60 soniyada, xavf (alarm) paytida esa har 3 soniyada yuboriladi.

