#include <RTClib.h>
#include <SPI.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <ezButton.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 13
#define Data_PIN 11
#define CS_PIN 10

#define D_Suhu A3
#define bp_set 2
#define bp_up 4
#define bp_down 3
#define buzzerPin 5
#define LDRPin A2
const float BETA = 3950;
RTC_DS1307 rtc;

ezButton bt_up(bp_up);
ezButton bt_set(bp_set);
ezButton bt_down(bp_down);

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);



const char* shortMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
struct ButtonHoldStatus {
  bool is_hold;
  unsigned long holdPressedTime;
  unsigned long lastRepeatTime;
};

struct Alarm {
  uint8_t hour;
  uint8_t minute;
  bool enabled;
  const char* pesan;
  bool flag;
};

ButtonHoldStatus holdStatusUp = {false, 0, 0};
ButtonHoldStatus holdStatusDown = {false, 0, 0};
ButtonHoldStatus holdStatusSet = {false, 0,0};
enum State {
	RUN,
	ADJUST_TIME,
  ADJUST_DATE,
  TRIGGER_ALARM_1,
  TRIGGER_ALARM_2,
  TRIGGER_ALARM_0,
  ADJUST_ALARM
};

bool single_click(ezButton &button){
	if(button.isPressed()){
		return true;
	}
	return false;
}


bool double_click(ezButton &button){
  static unsigned long lastPressTime = 0;
  static bool buttonWasReleased = true;
  const unsigned long double_click_interval = 250;

  if(button.isPressed() && buttonWasReleased){
    unsigned long currentTime = millis();
    if((currentTime - lastPressTime) < double_click_interval){
      buttonWasReleased = false; // Hindari deteksi double click berulang
      return true;
    }
    lastPressTime = currentTime;
  }
  if(button.isReleased()){
    buttonWasReleased = true;
  }
  return false;
}



bool hold_click(ezButton &button, ButtonHoldStatus &status){
  const unsigned long hold_click_interval = 1500;
  const unsigned long repeat_click_interval = 100;

  if (button.isPressed() && !status.is_hold){
    status.holdPressedTime = millis();
    status.lastRepeatTime = status.holdPressedTime;
    status.is_hold = true;
  } else if (button.isReleased()){
    status.is_hold = false;
  }
  if(status.is_hold){
    unsigned long currentTime = millis();
    if(currentTime - status.holdPressedTime >= hold_click_interval){
      if(currentTime - status.lastRepeatTime >= repeat_click_interval){
        status.lastRepeatTime = currentTime;
        return true;
      }
    }
  }
  return false;
}

bool hold_click_once(ezButton &button, ButtonHoldStatus &status){
  const unsigned long hold_click_interval = 1500;

  if(button.isPressed() && !status.is_hold){
    status.holdPressedTime = millis();
    status.is_hold = true;
  } else if(button.isReleased()){
    status.is_hold = false;
  }
  if(status.is_hold){
    unsigned long currentTime = millis();
    if(currentTime - status.holdPressedTime >= hold_click_interval){
      status.is_hold = false;
      return true;
    }
  }
  return false;
}


State currentState = RUN;
MD_MAX72XX::fontType_t newFont[] PROGMEM = 
{
	0, 	// 0                   
	0, 	// 1                   
	0, 	// 2                   
	0, 	// 3                   
	0, 	// 4                   
	0, 	// 5                   
	0, 	// 6                   
	0, 	// 7                   
	0, 	// 8                   
	0, 	// 9                   
	0, 	// 10                   
	0, 	// 11                   
	0, 	// 12                   
	0, 	// 13                   
	0, 	// 14                   
	0, 	// 15                   
	0, 	// 16                   
	0, 	// 17                   
	0, 	// 18                   
	0, 	// 19                   
	0, 	// 20                   
	0, 	// 21                   
	0, 	// 22                   
	0, 	// 23                   
	0, 	// 24                   
	0, 	// 25                   
	0, 	// 26                   
	0, 	// 27                   
	0, 	// 28                   
	0, 	// 29                   
	0, 	// 30                   
	0, 	// 31                   
	1, 0, 	// 32          
	0, 	// 33                   
	0, 	// 34                   
	0, 	// 35                   
	0, 	// 36                   
	0, 	// 37                   
	0, 	// 38                   
	0, 	// 39                   
	0, 	// 40                   
	0, 	// 41                   
	0, 	// 42                   
	0, 	// 43                   
	0, 	// 44                   
	1, 0, 	// 45                   
	1, 64, 	// 46                   
	3, 96, 24, 6, 	// 47                   
	3, 126, 66, 126, 	// 48                  
	3, 68, 126, 64, 	// 49                  
	3, 98, 82, 78, 	// 50                  
	3, 82, 82, 126, 	// 51                  
	3, 30, 16, 126, 	// 52                  
	3, 94, 82, 114, 	// 53                  
	3, 126, 82, 114, 	// 54                  
	3, 2, 114, 14, 	// 55                  
	3, 126, 82, 126, 	// 56                  
	3, 94, 82, 126, 	// 57                  
	1, 36, 	// 58                  
	2, 64, 36, 	// 59                   
	1, 255, 	// 60                   
	0, 	// 61                   
	1, 0, 	// 62                   
	4, 0, 0, 0, 0, 	// 63                   
	3, 0, 0, 0, 	// 64                   
	4, 124, 18, 18, 124, 	// 65                   
	4, 126, 74, 74, 52, 	// 66                   
	4, 60, 66, 66, 66, 	// 67                   
	4, 126, 66, 66, 60, 	// 68                   
	4, 126, 82, 82, 82, 	// 69                   
	4, 126, 18, 18, 2, 	// 70                   
	4, 60, 66, 82, 116, 	// 71                   
	4, 126, 16, 16, 126, 	// 72                   
	3, 66, 126, 66, 	// 73                   
	4, 34, 66, 66, 62, 	// 74                   
	4, 126, 24, 36, 66, 	// 75                   
	4, 126, 64, 64, 64, 	// 76                   
	4, 126, 4, 4, 126, 	// 77                   
	4, 126, 4, 8, 126, 	// 78                   
	4, 60, 66, 66, 60, 	// 79                   
	4, 126, 18, 18, 12, 	// 80                   
	4, 60, 66, 98, 124, 	// 81                   
	4, 126, 18, 18, 108, 	// 82                   
	4, 76, 82, 82, 98, 	// 83                   
	3, 2, 126, 2, 	// 84                   
	4, 62, 64, 64, 62, 	// 85                   
	4, 30, 32, 64, 62, 	// 86                   
	5, 62, 64, 48, 64, 62, 	// 87                   
	4, 110, 16, 16, 110, 	// 88                   
	4, 14, 16, 112, 14, 	// 89                   
	4, 98, 82, 74, 70, 	// 90                   
	0, 	// 91                   
	0, 	// 92                   
	0, 	// 93                   
	0, 	// 94                   
	0, 	// 95                   
	0, 	// 96                   
	3, 116, 84, 124, 	// 97                   
	3, 124, 80, 112, 	// 98                   
	3, 120, 72, 72, 	// 99                   
	3, 112, 80, 124, 	// 100                   
	3, 124, 84, 92, 	// 101                   
	3, 124, 20, 4, 	// 102                   
	3, 92, 84, 124, 	// 103                   
	3, 124, 16, 112, 	// 104                   
	3, 0, 116, 0, 	// 105                   
	3, 0, 64, 60, 	// 106                   
	3, 124, 48, 72, 	// 107                   
	3, 0, 126, 0, 	// 108                   
	3, 124, 8, 124, 	// 109                   
	3, 124, 4, 124, 	// 110                   
	3, 124, 68, 124, 	// 111                   
	3, 124, 20, 28, 	// 112                   
	3, 28, 20, 124, 	// 113                   
	3, 124, 4, 4, 	// 114                   
	3, 92, 84, 116, 	// 115                   
	3, 8, 60, 72, 	// 116                   
	3, 124, 64, 124, 	// 117                   
	3, 60, 64, 60, 	// 118                   
	4, 60, 96, 96, 60, 	// 119                   
	3, 108, 16, 108, 	// 120                   
	3, 92, 80, 124, 	// 121                   
	3, 100, 84, 92, 	// 122                   
	0, 	// 123                   
	1, 126, 	// 124                   
	0, 	// 125                   
	0, 	// 126                   
	0, 	// 127   
	3, 254, 194, 254, 	// 128   
	3, 196, 254, 192, 	// 129   
	3, 226, 210, 206, 	// 130   
	3, 210, 210, 254, 	// 131   
	3, 158, 144, 254, 	// 132   
	3, 222, 210, 242, 	// 133   
	3, 254, 210, 242, 	// 134   
	3, 130, 242, 142, 	// 135   
	3, 254, 210, 254, 	// 136   
	3, 222, 210, 254, 	// 137   
	0, 	// 138   
	0, 	// 139   
	0, 	// 140   
	0, 	// 141   
	0, 	// 142   
	0, 	// 143   
	0, 	// 144   
	4, 252, 146, 146, 252, 	// 145                   
	0, 	// 146                   
	0, 	// 147                   
	4, 254, 194, 194, 188, 	// 148                   
	0, 	// 149                   
	4, 254, 146, 146, 130, 	// 150                   
	0, 	// 151                   
	0, 	// 152                   
	0, 	// 153                   
	4, 162, 194, 194, 190, 	// 154                   
	0, 	// 155                   
	0, 	// 156                   
	4, 254, 132, 132, 254, 	// 157                   
	0, 	// 158                   
	4, 188, 194, 194, 188, 	// 159                   
	0, 	// 160                   
	0, 	// 161                   
	0, 	// 162                   
	4, 204, 210, 210, 226, 	// 163                   
	0, 	// 164                   
	0, 	// 165                   
	0, 	// 166                   
	0, 	// 167                   
	0, 	// 168                   
	0, 	// 169                   
	0, 	// 170                   
	0, 	// 171                   
	0, 	// 172                   
	0, 	// 173                   
	0, 	// 174                   
	0, 	// 175                   
	0, 	// 176                   
	3, 244, 212, 252, 	// 177                   
	3, 252, 208, 240, 	// 178                   
	3, 248, 200, 200, 	// 179                   
	0, 	// 180                   
	3, 252, 212, 220, 	// 181                   
	3, 252, 148, 132, 	// 182                   
	3, 220, 212, 252, 	// 183                   
	0, 	// 184                   
	0, 	// 185                   
	0, 	// 186                   
	0, 	// 187                   
	3, 128, 254, 128, 	// 188                   
	0, 	// 189                   
	3, 252, 132, 252, 	// 190                   
	3, 252, 196, 252, 	// 191                   
	3, 252, 148, 156, 	// 192                   
	0, 	// 193                   
	3, 252, 132, 132, 	// 194                   
	3, 92, 84, 116, 	// 195                   
	3, 136, 188, 200, 	// 196                   
	3, 252, 192, 252, 	// 197                   
	3, 188, 192, 188, 	// 198                   
	0, 	// 199                   
	0, 	// 200                   
	3, 220, 208, 252, 	// 201                   
	0, 	// 202    
	0, 	// 203   
	0, 	// 204   
	0, 	// 205   
	0, 	// 206   
	0, 	// 207   
	0, 	// 208   
	0, 	// 209   
	0, 	// 210   
	0, 	// 211   
	0, 	// 212   
	0, 	// 213   
	0, 	// 214   
	0, 	// 215   
	0, 	// 216   
	0, 	// 217   
	0, 	// 218   
	0, 	// 219   
	0, 	// 220   
	0, 	// 221   
	0, 	// 222   
	0, 	// 223   
	0, 	// 224   
	0, 	// 225   
	0, 	// 226   
	0, 	// 227   
	0, 	// 228   
	0, 	// 229   
	0, 	// 230   
	0, 	// 231   
	0, 	// 232   
	0, 	// 233   
	0, 	// 234   
	0, 	// 235   
	0, 	// 236   
	0, 	// 237   
	0, 	// 238   
	0, 	// 239   
	0, 	// 240   
	0, 	// 241   
	0, 	// 242   
	0, 	// 243   
	0, 	// 244   
	0, 	// 245   
	0, 	// 246   
	0, 	// 247   
	0, 	// 248   
	0, 	// 249   
	0, 	// 250   
	0, 	// 251   
	0, 	// 252   
	0, 	// 253   
	0, 	// 254   
	0, 	// 255
};

void getSuhu(char suhuStr[]) {
  static unsigned long lastUpdateTime = 0; // Waktu terakhir pembacaan diperbarui
  static int lastStableReading = 0;        // Pembacaan stabil terakhir
  const long updateInterval = 1000;        // Interval waktu untuk memperbarui pembacaan (1 detik)

  int analogValue = analogRead(D_Suhu);
  float voltage = analogValue * (5.0 / 1023.0);
  int currentReading = static_cast<int>(voltage * 100.0); // Pembacaan saat ini

  // Periksa apakah sudah waktunya untuk memperbarui pembacaan
  if (millis() - lastUpdateTime >= updateInterval) {
    // Perbarui pembacaan stabil dan waktu terakhir pembacaan diperbarui
    lastStableReading = currentReading;
    lastUpdateTime = millis();
  }

  // Gunakan pembacaan stabil terakhir untuk output
  snprintf(suhuStr, 4, "%d", lastStableReading); // Gunakan buffer yang cukup untuk tiga digit
}

void getTime(char timeStr[]) {
  DateTime now = rtc.now();

  // Mengonversi nilai waktu menjadi string dalam format HH:MM:SS
  snprintf(timeStr, 9, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
}

void getDate(char dateStr[]) {
  DateTime now = rtc.now();

  // Mendapatkan tiga huruf pertama dari nama bulan
  const char* monthChar = shortMonths[now.month() - 1];

  // Mengonversi nilai tanggal menjadi string dalam format YYYY-MMM-DD
  snprintf(dateStr, 12, "%02d-%s-%02d", now.day(), monthChar, now.year() % 100);
}

void tampil(const char* data) {
  // myDisplay.displayClear();
  myDisplay.displayText(data, PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
  myDisplay.displayAnimate();
}



Alarm alarms[3] = {
  {0, 0, false, "5024211001"},
  {0, 0, false, "5024211001 - Reza Ali Nirwansyah"},
  {0, 0, false, "Alarm aktif"}
};

bool check_alarm(Alarm& alarm, DateTime now) {
  if (alarm.enabled && alarm.flag) {
    if (alarm.hour == now.hour() && alarm.minute == now.minute()) {
      alarm.flag = false; // Set flag menjadi false setelah return true
      return true;
    }
  } else if (alarm.minute != now.minute()) {
    alarm.flag = true; // Reset flag menjadi true jika menit berubah
  }
  return false;
}

bool isBuzzerPlay = true;
unsigned long prevMillis_buzzer = 0;
int currentNote = 0;

const int alarmSong[] = {HIGH, 200, LOW, 200, HIGH, 200, LOW, 800};
const int alarmSongSize = sizeof(alarmSong) / sizeof(alarmSong[0]);

void playBuzzer() {
  if (isBuzzerPlay) {
    if (millis() - prevMillis_buzzer >= alarmSong[currentNote + 1]) {
      // If it's time to change the note
      if (alarmSong[currentNote] == HIGH) {
        tone(buzzerPin, 1000); // Ganti frekuensi sesuai kebutuhan
      } else {
        noTone(buzzerPin);
      }

      prevMillis_buzzer = millis();
      currentNote += 2; // Menggeser indeks dua kali karena setiap nada memiliki dua elemen (nada dan durasi)

      if (currentNote >= alarmSongSize) {
        // Reset to the beginning of the melody
        currentNote = 0;
      }
    }
  } else {
    noTone(buzzerPin);
  }
}

void TriggerAlarm(Alarm alarm) {
  bool isButtonPressed = false; // Variabel untuk menandai apakah tombol sudah ditekan

  myDisplay.displayText(alarm.pesan, PA_LEFT, 80, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while(!myDisplay.displayAnimate()) {
    if (isBuzzerPlay) {
      playBuzzer(); // Pastikan playBuzzer hanya dipanggil jika isBuzzerPlay true
    }
    if(digitalRead(bp_set) == LOW) { // Periksa apakah tombol ditekan
      Serial.println("Tombol sudah ditekan");
      isBuzzerPlay = false; 
      noTone(buzzerPin); // Matikan buzzer jika tombol ditekan
      myDisplay.displayClear(); // Bersihkan display
      isButtonPressed = true; // Tandai bahwa tombol sudah ditekan
      break; // Keluar dari loop while
    }
  }
  myDisplay.displayReset(); // Reset display setelah loop

  if(isButtonPressed){
    currentState = RUN;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  if (!rtc.begin()){
    Serial.println("rtc ndak nemu");
    while(1);
  }

	bt_set.setDebounceTime(50);
	bt_up.setDebounceTime(50);
	bt_down.setDebounceTime(50);

  myDisplay.begin();
  myDisplay.setFont(newFont);
  myDisplay.setIntensity(5);
  myDisplay.displayClear();
  myDisplay.setCharSpacing(1);

}

void loop() {
  DateTime now = rtc.now();
	bt_up.loop();
	bt_set.loop();
	bt_down.loop();

  int brightness;
  int ldrValue = analogRead(LDRPin);
  brightness = map(ldrValue, 0, 1023, 0, 15);
  myDisplay.setIntensity(brightness);

	switch(currentState){
		case RUN:
			run(now);
			break;
		case ADJUST_TIME:
			adjust_time();
			break;
    case ADJUST_DATE:
      adjust_date();
      break;
    case TRIGGER_ALARM_1:
      TriggerAlarm(alarms[1]);
      break;
    case TRIGGER_ALARM_2:
      TriggerAlarm(alarms[2]);
      break;
    case TRIGGER_ALARM_0:
      TriggerAlarm(alarms[0]);
      break;
    case ADJUST_ALARM:
      adjust_alarm();
      break;
	}
}

void run(DateTime now){

	char suhu[3];
	char time[9];
	char date[12];
	getTime(time);
	getSuhu(suhu);
	getDate(date);

  if(check_alarm(alarms[0], now)){
    currentState = TRIGGER_ALARM_0;
  }
  if(check_alarm(alarms[1], now)){
    currentState = TRIGGER_ALARM_1;
  }
  if(check_alarm(alarms[2], now)){
    currentState = TRIGGER_ALARM_2;
  }

	int sekon = now.second();

if ((10 <= sekon && sekon <= 12) || (40 <= sekon && sekon <= 42)) {
  tampil(suhu);
} else if ((13 <= sekon && sekon <= 15) || (43 <= sekon && sekon <= 45)) {
  tampil(date);
} else {
  tampil(time);
}
	if(hold_click_once(bt_set, holdStatusSet)){
		currentState = ADJUST_TIME;
	}
}



void adjust_time(){
	static uint8_t jam = 0;
	static uint8_t menit = 0;
	static uint8_t detik = 0;
	static uint8_t posisi = 0;
	static bool adjust_time_init = true;

  if(adjust_time_init){
    DateTime now = rtc.now();
    jam = now.hour();
    menit = now.minute();
    detik = now.second();
    adjust_time_init = false;
  }

	char waktu[11];

	if (single_click(bt_up)){
		if(posisi == 0){
		  jam = (jam == 23) ? 23 : jam + 1; // Mengurangi jam dan memastikan tidak kurang dari 0
	  } else if(posisi == 1){
		  menit = (menit == 59) ? 59 : menit + 1; // Mengurangi menit dan memastikan tidak kurang dari 0
	  } else if(posisi == 2){
		  detik = (detik == 59) ? 59 : detik + 1; // Mengurangi detik dan memastikan tidak kurang dari 0
	  }
	}

	if (hold_click(bt_up, holdStatusUp)){
		if(posisi == 0){
      jam = (jam == 23) ? 23 : jam + 1; // Mengurangi jam dan memastikan tidak kurang dari 0
    } else if(posisi == 1){
      menit = (menit == 59) ? 59 : menit + 1; // Mengurangi menit dan memastikan tidak kurang dari 
    } else if(posisi == 2){
      detik = (detik == 59) ? 59 : detik + 1; // Mengurangi detik dan memastikan tidak kurang dari 0
    }
	}

  if (single_click(bt_down)){
	if(posisi == 0){
		jam = (jam == 0) ? 0 : jam - 1; // Mengurangi jam dan memastikan tidak kurang dari 0
	} else if(posisi == 1){
		menit = (menit == 0) ? 0 : menit - 1; // Mengurangi menit dan memastikan tidak kurang dari 0
	} else if(posisi == 2){
		detik = (detik == 0) ? 0 : detik - 1; // Mengurangi detik dan memastikan tidak kurang dari 0
	}
}

if (hold_click(bt_down, holdStatusDown)){
	if(posisi == 0){
		jam = (jam == 0) ? 0 : jam - 1; // Mengurangi jam dan memastikan tidak kurang dari 0
	} else if(posisi == 1){
		menit = (menit == 0) ? 0 : menit - 1; // Mengurangi menit dan memastikan tidak kurang dari 0
	} else if(posisi == 2){
		detik = (detik == 0) ? 0 : detik - 1; // Mengurangi detik dan memastikan tidak kurang dari 0
	}
}

	if (single_click(bt_set)){
		posisi = (posisi + 1) % 3; // Mengubah posisi antara jam, menit, dan detik
	}

	if(double_click(bt_set)){	

    DateTime now = rtc.now();
    currentState =ADJUST_DATE;
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), jam, menit, detik));
    adjust_time_init = true;
	}
  if (hold_click_once(bt_set, holdStatusSet)){
    currentState = RUN;
  }


	// sprintf(waktu, "%02d:%02d:%02d", jam, menit, detik);
  if(posisi == 0){
    sprintf(waktu, "%c%c:%02d:%02d", jam / 10 + '0' + 80, jam % 10 + '0' + 80, menit, detik);
  } else if(posisi == 1){
      sprintf(waktu, "%02d:%c%c:%02d", jam, menit / 10 + '0' + 80, menit % 10 + '0' + 80, detik);
  } else if(posisi == 2){
      sprintf(waktu, "%02d:%02d:%c%c", jam, menit, detik / 10 + '0' + 80, detik % 10 + '0' + 80);
  }
	tampil(waktu); // Asumsi bahwa fungsi tampil() ada dan dapat menampilkan waktu
}

void adjust_date() {
  static uint8_t tahun = 0;
  static uint8_t bulan = 0;
  static uint8_t tanggal = 0;
  static uint8_t posisi = 0; // 0 untuk tahun, 1 untuk bulan, 2 untuk tanggal
  static bool adjust_date_init = true;
  myDisplay.setCharSpacing(1);

  if (adjust_date_init) {
    DateTime now = rtc.now();
    tahun = now.year();
    bulan = now.month();
    tanggal = now.day();
    adjust_date_init = false;
  }

  char dateStr[11];

  if (single_click(bt_up)) {
    if (posisi == 0) {
      tahun++;
    } else if (posisi == 1) {
      bulan = (bulan == 12) ? 1 : bulan + 1;
    } else if (posisi == 2) {
      tanggal = (tanggal == 31) ? 1 : tanggal + 1;
    }
  }

  if (hold_click(bt_up, holdStatusUp)) {
    if (posisi == 0) {
      tahun++;
    } else if (posisi == 1) {
      bulan = (bulan == 12) ? 1 : bulan + 1;
    } else if (posisi == 2) {
      tanggal = (tanggal == 31) ? 1 : tanggal + 1;
    }
  }

  if (single_click(bt_down)) {
    if (posisi == 0) {
      tahun--;
    } else if (posisi == 1) {
      bulan = (bulan == 1) ? 12 : bulan - 1;
    } else if (posisi == 2) {
      tanggal = (tanggal == 1) ? 31 : tanggal - 1;
    }
  }

  if (hold_click(bt_down, holdStatusDown)) {
    if (posisi == 0) {
      tahun--;
    } else if (posisi == 1) {
      bulan = (bulan == 1) ? 12 : bulan - 1;
    } else if (posisi == 2) {
      tanggal = (tanggal == 1) ? 31 : tanggal - 1;
    }
  }

  if (single_click(bt_set)) {
    posisi = (posisi + 1) % 3;
  }

  if (double_click(bt_set)) {
    DateTime now = rtc.now();
    rtc.adjust(DateTime(tahun, bulan, tanggal, now.hour(), now.minute(), now.second()));
    currentState = ADJUST_ALARM;
    adjust_date_init = true;
  }
  if (hold_click_once(bt_set, holdStatusSet)){
    currentState = RUN;
  }


  char* monthChar = shortMonths[bulan - 1];
  // snprintf(dateStr, sizeof(dateStr), "%02d-%s-%02d", tanggal, monthChar, tahun%100);
  if(posisi == 0){
    snprintf(dateStr, sizeof(dateStr), "%02d-%s-%c%c",tanggal, monthChar,  (tahun % 100) / 10 + '0' + 80, tahun % 10 + '0' + 80);
  } else if(posisi == 1){
      snprintf(dateStr, sizeof(dateStr), "%02d-%c%c%c-%02d", tanggal, monthChar[0] + 80, monthChar[1] + 80, monthChar[2] + 80, tahun % 100);
  } else if(posisi == 2){
      snprintf(dateStr, sizeof(dateStr), "%c%c-%s-%02d", tanggal / 10 + '0' + 80, tanggal % 10 + '0' + 80, monthChar, tahun%100);
  }
  // snprintf(dateStr, "p%sp", monthChar);
  tampil(dateStr); // Asumsi bahwa fungsi tampil() ada dan dapat menampilkan tanggal
}

void adjust_alarm(){
  static bool adjust_alarm_init = true;
  static uint8_t no_alarm = 0;
  static uint8_t posisi = 0;
  static uint8_t jam = alarms[no_alarm].hour;
  static uint8_t menit = alarms[no_alarm].minute;
  static bool aktif = alarms[no_alarm].enabled;

  if(adjust_alarm_init){
    no_alarm = 0;
    adjust_alarm_init = false;
    jam = alarms[no_alarm].hour;
    menit = alarms[no_alarm].minute;
    aktif = alarms[no_alarm].enabled;
  }


  if (single_click(bt_up)){
    if(posisi == 0){
      aktif = true;
    } else if(posisi == 1){
      jam = (jam == 23) ? 23 : jam + 1; // Mengurangi jam dan memastikan tidak kurang dari 0
	  } else if(posisi == 2){
		  menit = (menit == 59) ? 59 : menit + 1; // Mengurangi menit dan memastikan tidak kurang dari 0
	  }
  }

  if (hold_click(bt_up, holdStatusUp)){
    if(posisi == 1){
      jam = (jam == 23) ? 23 : jam + 1; // Mengurangi jam dan memastikan tidak kurang dari 0
	  } else if(posisi == 2){
		  menit = (menit == 59) ? 59 : menit + 1; // Mengurangi menit dan memastikan tidak kurang dari 0
	  }
  }

  if (single_click(bt_down)){
    if(posisi == 0){
      aktif = false;
    } else if(posisi == 1){
      jam = (jam == 0) ? 0 : jam - 1; // Mengurangi jam dan memastikan tidak kurang dari 0
    } else if(posisi == 2){
      menit = (menit == 0) ? 0 : menit - 1; // Mengurangi menit dan memastikan tidak kurang dari 0
    }
  }

  if (hold_click(bt_down, holdStatusDown)){
    if(posisi == 1){
      jam = (jam == 0) ? 0 : jam - 1; // Mengurangi jam dan memastikan tidak kurang dari 0
    } else if(posisi == 2){
      menit = (menit == 0) ? 0 : menit - 1; // Mengurangi menit dan memastikan tidak kurang dari 0
    }
  }

  if (single_click(bt_set)){
    posisi = (posisi + 1) % 3;
  }

  if (double_click(bt_set)){
    alarms[no_alarm].hour = jam;
    alarms[no_alarm].minute = menit;
    alarms[no_alarm].enabled = aktif;
    no_alarm++;
    jam = alarms[no_alarm].hour;
    menit = alarms[no_alarm].minute;
    aktif = alarms[no_alarm].enabled;
    if(no_alarm == 3){
      adjust_alarm_init = true;
      currentState = RUN;
    }
  }

  if (hold_click_once(bt_set, holdStatusSet)){
    currentState = RUN;
  }

  char* tanda;
  if(aktif){
    tanda = "on";
  } else {
    tanda = "of";
  }

  char jamString[3];
  sprintf(jamString, "%02d", jam);
  Serial.println(jamString);

  char menitString[3];
  sprintf(menitString, "%02d", menit);
  Serial.println(menitString);

  char alarmTime[11];
  if (posisi == 0){
    sprintf(alarmTime,"%d%c%c%s.%02d",no_alarm,tanda[0] + 80, tanda[1] + 80,jamString,menit);
  } else if(posisi == 1){
    sprintf(alarmTime,"%d%c%c%c%c.%02d",no_alarm,tanda[0], tanda[1],jamString[0] + 80, jamString[1] + 80,menit);
  } else if(posisi == 2){
    sprintf(alarmTime,"%d%c%c%c%c.%c%c",no_alarm,tanda[0], tanda[1],jamString[0], jamString[1],menitString[0] + 80, menitString[1] + 80);
  }
  tampil(alarmTime);
}

