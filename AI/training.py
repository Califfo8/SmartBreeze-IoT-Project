import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error, mean_absolute_error
import xgboost as xgb
import matplotlib.pyplot as plt

# Step 1: Caricamento dei dati
data = pd.read_csv('/path/to/solar_data.csv')

# Step 2: Pre-elaborazione dei dati
data = data.dropna()  # Rimuove i valori mancanti
X = data[['temperature', 'humidity', 'solar_irradiance']]  # Caratteristiche
y = data['production']  # Target

# Step 3: Suddivisione dei dati
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Step 4: Addestramento del modello con XGBoost
model = xgb.XGBRegressor(objective='reg:squarederror', n_estimators=100, learning_rate=0.1)
model.fit(X_train, y_train)

# Step 5: Predizione e valutazione
y_pred = model.predict(X_test)
rmse = np.sqrt(mean_squared_error(y_test, y_pred))
mae = mean_absolute_error(y_test, y_pred)

print(f'RMSE: {rmse}')
print(f'MAE: {mae}')

# Step 6: Visualizzazione dei risultati
plt.scatter(y_test, y_pred)
plt.xlabel('True Values')
plt.ylabel('Predictions')
plt.title('True Values vs Predictions')
plt.show()
