#include <PsxControllerBitBang.h>
#include <NintendoSwitchControlLibrary.h>

/* We must use the bit-banging interface, as SPI pins are only available on the
 * ICSP header on the Leonardo.
 *
 */
const byte PIN_PS2_ATT = 11;
const byte PIN_PS2_CMD = 9;
const byte PIN_PS2_DAT = 8;
const byte PIN_PS2_CLK = 10;

// タクトスイッチ用のピン定義
const byte PIN_TACT_LEFT = 2;   // 左ボタン用タクトスイッチ
const byte PIN_TACT_RIGHT = 3;  // 右ボタン用タクトスイッチ

const unsigned long POLLING_INTERVAL = 1000U / 50U;

// Send debug messages to serial port
// #define ENABLE_SERIAL_DEBUG

PsxControllerBitBang<PIN_PS2_ATT, PIN_PS2_CMD, PIN_PS2_DAT, PIN_PS2_CLK> psx;

#ifdef ENABLE_SERIAL_DEBUG
	#define dstart(spd) do {Serial.begin (spd); while (!Serial) {digitalWrite (LED_BUILTIN, (millis () / 500) % 2);}} while (0);
	#define debug(...) Serial.print (__VA_ARGS__)
	#define debugln(...) Serial.println (__VA_ARGS__)
#else
	#define dstart(...)
	#define debug(...)
	#define debugln(...)
#endif

boolean haveController = false;

// 前回のマスコンハンドル状態を記憶
int lastYAxis = ANALOG_IDLE_VALUE;
int lastBrakeState = 0; // 前回のブレーキ状態を記憶（0=解除、1-8=ブレーキ、10-20=非常ブレーキ、9は欠番）

// タクトスイッチ用のデバウンス処理
unsigned long lastTactLeftTime = 0;
unsigned long lastTactRightTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; // 50msのデバウンス時間
bool lastTactLeftState = HIGH;
bool lastTactRightState = HIGH;


#define toDegrees(rad) (rad * 180.0 / PI)

#define deadify(var, thres) (abs (var) > thres ? (var) : 0)


/** \brief Dead zone for analog sticks
 *
 * If the analog stick moves less than this value from the center position, it
 * is considered still.
 *
 * \sa ANALOG_IDLE_VALUE
 */
const byte ANALOG_DEAD_ZONE = 50U;

// ボタンの状態をデバッグ出力する関数
void debugButtons() {
	debug("Buttons: ");
	
	// 各ボタンの状態をチェック
	if (psx.buttonPressed(PSB_SQUARE)) debug("SQUARE ");
	if (psx.buttonPressed(PSB_CROSS)) debug("CROSS ");
	if (psx.buttonPressed(PSB_CIRCLE)) debug("CIRCLE ");
	if (psx.buttonPressed(PSB_TRIANGLE)) debug("TRIANGLE ");
	if (psx.buttonPressed(PSB_L1)) debug("L1 ");
	if (psx.buttonPressed(PSB_R1)) debug("R1 ");
	if (psx.buttonPressed(PSB_L2)) debug("L2 ");
	if (psx.buttonPressed(PSB_R2)) debug("R2 ");
	if (psx.buttonPressed(PSB_SELECT)) debug("SELECT ");
	if (psx.buttonPressed(PSB_START)) debug("START ");
	if (psx.buttonPressed(PSB_L3)) debug("L3 ");
	if (psx.buttonPressed(PSB_R3)) debug("R3 ");
	if (psx.buttonPressed(PSB_PAD_UP)) debug("PAD_UP ");
	if (psx.buttonPressed(PSB_PAD_DOWN)) debug("PAD_DOWN ");
	if (psx.buttonPressed(PSB_PAD_LEFT)) debug("PAD_LEFT ");
	if (psx.buttonPressed(PSB_PAD_RIGHT)) debug("PAD_RIGHT ");
	
	debugln();
}



void setup () {
	// Lit the builtin led whenever buttons are pressed
	pinMode (LED_BUILTIN, OUTPUT);

	// タクトスイッチのピンモードを設定（プルアップ有効）
	pinMode(PIN_TACT_LEFT, INPUT_PULLUP);
	pinMode(PIN_TACT_RIGHT, INPUT_PULLUP);

	dstart (115200);
  pushButton(Button::L, 500, 5);

	debugln (F("Ready!"));
}

void loop () {
	static unsigned long last = 0;
 
	if (millis () - last >= POLLING_INTERVAL) {
		last = millis ();
	 
		if (!haveController) {
			if (psx.begin ()) {
				debugln (F("Controller found!"));
				if (!psx.enterConfigMode ()) {
					debugln (F("Cannot enter config mode"));
				} else {
					// Try to enable analog sticks
					if (!psx.enableAnalogSticks ()) {
						debugln (F("Cannot enable analog sticks"));
					}
								 
					if (!psx.exitConfigMode ()) {
						debugln (F("Cannot exit config mode"));
					}
				}
			 
				haveController = true;
			}
		} else {
			if (!psx.read ()) {
				debugln (F("Controller lost :("));
				haveController = false;
			} else {
				// Switchコントローラーのボタンマッピング
				if (psx.buttonPressed(PSB_CROSS)) {
					SwitchControlLibrary().pressButton(Button::A);
				} else {
					SwitchControlLibrary().releaseButton(Button::A);
				}
				
				if (psx.buttonPressed(PSB_CIRCLE)) {
					SwitchControlLibrary().pressButton(Button::B);
				} else {
					SwitchControlLibrary().releaseButton(Button::B);
				}
				
				if (psx.buttonPressed(PSB_SQUARE)) {
					SwitchControlLibrary().pressButton(Button::X);
				} else {
					SwitchControlLibrary().releaseButton(Button::X);
				}
				
				if (psx.buttonPressed(PSB_SELECT)) {
					SwitchControlLibrary().pressButton(Button::MINUS);
				} else {
					SwitchControlLibrary().releaseButton(Button::MINUS);
				}
				
				if (psx.buttonPressed(PSB_START)) {
					SwitchControlLibrary().pressButton(Button::PLUS);
				} else {
					SwitchControlLibrary().releaseButton(Button::PLUS);
				}

				// タクトスイッチの処理（デバウンス付き）
				unsigned long currentTime = millis();
				static uint8_t currentHatState = Hat::NEUTRAL;
				uint8_t newHatState = Hat::NEUTRAL;
				
				// 左タクトスイッチの処理
				bool currentTactLeftState = digitalRead(PIN_TACT_LEFT);
				if (currentTactLeftState != lastTactLeftState) {
					lastTactLeftTime = currentTime;
				}
				if ((currentTime - lastTactLeftTime) > DEBOUNCE_DELAY) {
					if (currentTactLeftState == LOW) {
						newHatState = Hat::LEFT;
					}
				}
				lastTactLeftState = currentTactLeftState;
				
				// 右タクトスイッチの処理
				bool currentTactRightState = digitalRead(PIN_TACT_RIGHT);
				if (currentTactRightState != lastTactRightState) {
					lastTactRightTime = currentTime;
				}
				if ((currentTime - lastTactRightTime) > DEBOUNCE_DELAY) {
					if (currentTactRightState == LOW) {
						newHatState = Hat::RIGHT;
					}
				}
				lastTactRightState = currentTactRightState;
				
				// Hat状態の変更を処理
				if (newHatState != currentHatState) {
					if (newHatState != Hat::NEUTRAL) {
						SwitchControlLibrary().pressHatButton(newHatState);
					} else {
						SwitchControlLibrary().releaseHatButton();
					}
					currentHatState = newHatState;
				}

				// 電車でGoコントローラーのマスコンハンドル処理
				int yAxis = lastYAxis; // 前回の状態を初期値とする
				
				// レバー（加速）の処理 - TRIANGLEボタンとD-Padで5段階
				// 切: TRIANGLE release, PAD_UP, DOWN, LEFT, RIGHT press
				// 1: TRIANGLE, PAD_UP, DOWN, RIGHT press
				// 2: TRIANGLE release, PAD_UP, DOWN, RIGHT press
				// 3: TRIANGLE press, PAD_UP, DOWN, LEFT press
				// 4: TRIANGLE release, PAD_UP, DOWN, LEFT press
				// 5: TRIANGLE, PAD_UP, DOWN press
				bool trianglePressed = psx.buttonPressed(PSB_TRIANGLE);
				bool padUp = psx.buttonPressed(PSB_PAD_UP);
				bool padDown = psx.buttonPressed(PSB_PAD_DOWN);
				bool padLeft = psx.buttonPressed(PSB_PAD_LEFT);
				bool padRight = psx.buttonPressed(PSB_PAD_RIGHT);
				
				if (!trianglePressed && padUp && padDown && padLeft && padRight) {
					// 切: TRIANGLE release, PAD_UP, DOWN, LEFT, RIGHT press
					yAxis = ANALOG_IDLE_VALUE; // 128 (ニュートラル)
				} else if (trianglePressed && padUp && padDown && padRight && !padLeft) {
					// 段階1: TRIANGLE, PAD_UP, DOWN, RIGHT press
					yAxis = ANALOG_IDLE_VALUE + 30;
				} else if (!trianglePressed && padUp && padDown && padRight && !padLeft) {
					// 段階2: TRIANGLE release, PAD_UP, DOWN, RIGHT press
					yAxis = ANALOG_IDLE_VALUE + 51;
				} else if (trianglePressed && padUp && padDown && padLeft && !padRight) {
					// 段階3: TRIANGLE press, PAD_UP, DOWN, LEFT press
					yAxis = ANALOG_IDLE_VALUE + 76;
				} else if (!trianglePressed && padUp && padDown && padLeft && !padRight) {
					// 段階4: TRIANGLE release, PAD_UP, DOWN, LEFT press
					yAxis = ANALOG_IDLE_VALUE + 102;
				} else if (trianglePressed && padUp && padDown && !padLeft && !padRight) {
					// 段階5: TRIANGLE, PAD_UP, DOWN press
					yAxis = ANALOG_MAX_VALUE;
				}
				
				// ブレーキの処理 - L1/R1/L2/R2ボタンで8段階
				bool l1Pressed = psx.buttonPressed(PSB_L1);
				bool r1Pressed = psx.buttonPressed(PSB_R1);
				bool l2Pressed = psx.buttonPressed(PSB_L2);
				bool r2Pressed = psx.buttonPressed(PSB_R2);
				
				// ブレーキ段階の判定
				int currentBrakeState = 0; // 現在のブレーキ状態
				if (!l1Pressed && r1Pressed && l2Pressed && r2Pressed) {
					// 解除状態 - レバーの値を使用
					currentBrakeState = 0;
				} else if (l1Pressed && r1Pressed && !l2Pressed && r2Pressed) {
					// ブレーキ1段階
					currentBrakeState = 1;
					yAxis = ANALOG_IDLE_VALUE - 30;
				} else if (!l1Pressed && r1Pressed && !l2Pressed && r2Pressed) {
					// ブレーキ2段階
					currentBrakeState = 2;
					yAxis = ANALOG_IDLE_VALUE - 40;
				} else if (l1Pressed && !r1Pressed && l2Pressed && r2Pressed) {
					// ブレーキ3段階
					currentBrakeState = 3;
					yAxis = ANALOG_IDLE_VALUE - 60;
				} else if (!l1Pressed && !r1Pressed && l2Pressed && r2Pressed) {
					// ブレーキ4段階
					currentBrakeState = 4;
					yAxis = ANALOG_IDLE_VALUE - 75;
				} else if (l1Pressed && !r1Pressed && !l2Pressed && r2Pressed) {
					// ブレーキ5段階
					currentBrakeState = 5;
					yAxis = ANALOG_IDLE_VALUE - 80;
				} else if (!l1Pressed && !r1Pressed && !l2Pressed && r2Pressed) {
					// ブレーキ6段階
					currentBrakeState = 6;
					yAxis = ANALOG_IDLE_VALUE - 96;
				} else if (l1Pressed && r1Pressed && l2Pressed && !r2Pressed) {
					// ブレーキ7段階
					currentBrakeState = 7;
					yAxis = ANALOG_IDLE_VALUE - 112;
				} else if (!l1Pressed && r1Pressed && l2Pressed && !r2Pressed) {
					// ブレーキ8段階
					currentBrakeState = 8;
					yAxis = 1;
				} else if (l1Pressed && r1Pressed && !l2Pressed && !r2Pressed && lastBrakeState >= 8) {
					// 非常ブレーキ 10段階: L1, R1 press（8段階から直接遷移可能）
					currentBrakeState = 10;
					yAxis = 0;
				} else if (l1Pressed && r1Pressed && l2Pressed && r2Pressed && lastBrakeState >= 10) {
					// 非常ブレーキ 11, 13, 15, 17, 19段階 (全てpress)
					// 前回の状態が10段階以上の場合のみこの条件で処理
					// これにより通常ブレーキ（1-8段階）の切り替え時の誤入力を防ぐ
					if (lastBrakeState == 10 || lastBrakeState == 11) {
						currentBrakeState = 11;
					} else if (lastBrakeState == 11 || lastBrakeState == 12) {
						currentBrakeState = 13;
					} else if (lastBrakeState == 13 || lastBrakeState == 14) {
						currentBrakeState = 15;
					} else if (lastBrakeState == 15 || lastBrakeState == 16) {
						currentBrakeState = 17;
					} else if (lastBrakeState == 17 || lastBrakeState == 18) {
						currentBrakeState = 19;
					} else {
						currentBrakeState = lastBrakeState; // 前回の状態を維持
					}
					yAxis = 0;
				} else if (!l1Pressed && r1Pressed && !l2Pressed && !r2Pressed && lastBrakeState >= 10) {
					// 非常ブレーキ 12段階: R1 press
					currentBrakeState = 12;
					yAxis = 0;
				} else if (l1Pressed && !r1Pressed && l2Pressed && !r2Pressed && lastBrakeState >= 10) {
					// 非常ブレーキ 14段階: L1, L2 press
					currentBrakeState = 14;
					yAxis = 0;
				} else if (!l1Pressed && !r1Pressed && l2Pressed && !r2Pressed && lastBrakeState >= 10) {
					// 非常ブレーキ 16段階: L2 press
					currentBrakeState = 16;
					yAxis = 0;
				} else if (l1Pressed && !r1Pressed && !l2Pressed && !r2Pressed && lastBrakeState >= 10) {
					// 非常ブレーキ 18段階: L1 press
					currentBrakeState = 18;
					yAxis = 0;
				} else if (!l1Pressed && !r1Pressed && !l2Pressed && !r2Pressed && lastBrakeState >= 10) {
					// 非常ブレーキ 20段階: なし (全てrelease)
					currentBrakeState = 20;
					yAxis = 0;
				} else {
					// 完全な組み合わせが見つからない場合は前回の状態を維持
					currentBrakeState = lastBrakeState;
					if (lastBrakeState > 0) {
						// 前回がブレーキ状態だった場合は、その状態を維持
						switch (lastBrakeState) {
							case 1: yAxis = ANALOG_IDLE_VALUE - 30; break;
							case 2: yAxis = ANALOG_IDLE_VALUE - 40; break;
							case 3: yAxis = ANALOG_IDLE_VALUE - 60; break;
							case 4: yAxis = ANALOG_IDLE_VALUE - 75; break;
							case 5: yAxis = ANALOG_IDLE_VALUE - 80; break;
							case 6: yAxis = ANALOG_IDLE_VALUE - 96; break;
							case 7: yAxis = ANALOG_IDLE_VALUE - 112; break;
							case 8:
							case 10:
							case 11:
							case 12:
							case 13:
							case 14:
							case 15:
							case 16:
							case 17:
							case 18:
							case 19:
							case 20: yAxis = 1; break;
						}
					}
				}
				
				// 前回のブレーキ状態を更新
				lastBrakeState = currentBrakeState;
				
				// 非常ブレーキ状態（10段階以降）でZLをpress、それ以外でrelease
				if (currentBrakeState >= 10) {
					SwitchControlLibrary().pressButton(Button::ZL);
				} else {
					SwitchControlLibrary().releaseButton(Button::ZL);
				}
				
				// 前回の状態を更新
				lastYAxis = yAxis;

				// Switchコントローラーのアナログスティック設定
				// 左スティックのY軸にマスコンハンドルの値を設定
				// NintendoSwitchControlLibraryでは値は0-255の範囲
				int leftStickY = yAxis; // 既に0-255の範囲で計算済み
				SwitchControlLibrary().moveLeftStick(128, leftStickY);
				
				// 右スティックはニュートラル位置
				SwitchControlLibrary().moveRightStick(128, 128);

				// デバッグ出力
				debugButtons();

				// データを送信
				SwitchControlLibrary().sendReport();
			}
		}
	}
} 
