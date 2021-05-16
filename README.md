# Freelab aithinker-esp32-esp32s2

## IoT Platform
Tutorial ini menggunakan [https://platform.iotera.io/](url)

1. Buat akun di platform Iotera
2. Buat aplikasi baru (**Create Application**), masuk ke Aplikasi tersebut
3. Di menu bar sebelah kiri, pilih **Perangkat**, klik **+ Alat Baru**
4. Pilih **MQTT** dan masukkan nomor seri serta label (terserah). Klik **Simpan**
5. Sebaiknya bikin 2 perangkat (untuk Ai Thinker NodeMCU 32 dan NodeMCU 32 S2)
6. Di daftar Perangkat, _hover_ ke icon _Aksi_, pilih **Konfigurasi**

Konfigurasi untuk NodeMCU32: 
```json
{
	"led_onboard": {
		"channel": "CUSTOM",
		"command": {
			"turnon": "number"
		},
		"data": {
			"ledstat": "number"
		}
	}
}
```

Konfigurasi untuk NodeMCU32 S2:
```json
{
	"wifi_node": {
		"channel": "CUSTOM",
		"data": {
			"pulse_counter": "object",
			"pulse_counter__ID": "text",
			"pulse_counter__CH1": "number",
			"pulse_counter__CH2": "number",
			"battery": "number"
		}
	}
}
```
