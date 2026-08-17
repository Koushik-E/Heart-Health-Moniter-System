import warnings
import pandas as pd
import requests
import time
import datetime
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression

# Ignore sklearn feature name warnings
warnings.filterwarnings(
    "ignore",
    message="X does not have valid feature names, but LogisticRegression was fitted with feature names"
)

# =======================
# 1) Load & Train ML Model
# =======================
df = pd.read_csv("heart_attack_dataset.csv")

X = df[['Heartbeat_BPM', 'SpO2_Percent', 'Temperature_C']]
y = df['Risk_Level']

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=y
)

clf = LogisticRegression(max_iter=1000, random_state=42)
clf.fit(X_train, y_train)

print("✅ Logistic Regression model trained successfully.")
print(f"📊 Test Accuracy: {clf.score(X_test, y_test) * 100:.2f}%")

risk_dict = {0: "Low Risk", 1: "Moderate Risk", 2: "High Risk"}

# =======================
# 2) Fetch Data from ESP32
# =======================
def fetch_data_from_esp32(url="http://10.21.134.45//data"):
    try:
        response = requests.get(url, timeout=5)
        response.raise_for_status()

        try:
            data = response.json()
        except ValueError:
            print("❌ Invalid JSON from ESP32")
            return None

        hb = float(data.get("bpm", 0))
        spo2 = float(data.get("spO2", 0))
        temp = float(data.get("temp", 0))

        return hb, spo2, temp

    except requests.exceptions.RequestException as e:
        print("❌ Could not fetch data:", e)
        return None

# =======================
# 3) Prediction + Status
# =======================
def predict_risk(hb, spo2, temp):
    # Prepare ML input
    user_data = pd.DataFrame([[hb, spo2, temp]],
                             columns=['Heartbeat_BPM', 'SpO2_Percent', 'Temperature_C'])
    prediction = clf.predict(user_data)[0]

    # Timestamp
    now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"\n⏰ Time: {now}")
    print("📟 Live Data from Device:")
    print(f"   ❤️ Heartbeat: {hb:.2f} BPM")
    print(f"   🫁 SpO₂: {spo2:.2f}%")
    print(f"   🌡️ Temperature: {temp:.2f} °C")

    # Heartbeat check
    if hb < 60:
        print("⚠️ Heartbeat is LOW (possible bradycardia)")
    elif 60 <= hb <= 100:
        print("✅ Heartbeat is NORMAL")
    elif 101 <= hb <= 120:
        print("⚠️ Heartbeat is SLIGHTLY HIGH")
    else:
        print("🚨 Heartbeat is VERY HIGH (possible tachycardia)")

    # SpO2 check
    if spo2 < 90:
        print("🚨 SpO₂ is VERY LOW (possible hypoxemia)")
    elif 90 <= spo2 <= 94:
        print("⚠️ SpO₂ is SLIGHTLY LOW")
    elif 95 <= spo2 <= 100:
        print("✅ SpO₂ is NORMAL")
    else:
        print("⚠️ SpO₂ value seems unrealistic")

    # Temperature check
    if temp < 36:
        print("⚠️ Temperature is LOW (possible hypothermia)")
    elif 36 <= temp <= 37.5:
        print("✅ Temperature is NORMAL (no fever)")
    elif 37.6 <= temp <= 38.5:
        print("⚠️ Temperature is SLIGHTLY HIGH (mild fever)")
    else:
        print("🚨 Temperature is VERY HIGH (high fever)")

    # ML Prediction
    print(f"\n🧠 Predicted Heart Attack Risk: {prediction} ({risk_dict[prediction]})")

# =======================
# 4) Main Loop
# =======================
try:
    while True:
        data = fetch_data_from_esp32("http://10.21.134.45//data")  # use your ESP32 IP
        if data:
            predict_risk(*data)
        time.sleep(5)  # fetch every 5 sec
except KeyboardInterrupt:
    print("\n🛑 Program stopped by user.")
