# 🌅 ESP32 Sunrise Alarm Clock (Despertador Amanecer)

Un proyecto de domótica basado en **ESP32** para simular un amanecer natural utilizando tiras o módulos LED de 5 canales (RGB + Blanco Cálido + Blanco Frío). El sistema incluye un servidor web integrado que permite configurar la hora de la alarma, controlar las luces manualmente y activar un modo de lectura, todo desde cualquier navegador en tu red local.

## ✨ Características

*   **Simulación de Amanecer (45 minutos):** Transición suave de colores oscuros/rojizos a blancos brillantes para un despertar progresivo y natural.
*   **Servidor Web Integrado:** Interfaz de usuario moderna y responsiva (HTML/CSS/JS) sin necesidad de aplicaciones externas.
*   **Sincronización NTP:** Obtiene la hora exacta automáticamente de internet (`pool.ntp.org`).
*   **Control Manual (Lámpara):** Sliders independientes para los canales R, G, B, WW (Warm White) y CW (Cold White).
*   **Modo Lectura:** Un botón rápido que establece una luz cálida ideal para leer antes de dormir.
*   **IP Estática:** Configurado para mantener siempre la misma dirección IP en tu red local.

## 🛠️ Requisitos de Hardware

1.  **Microcontrolador:** ESP32 (NodeMCU o similar).
2.  **Iluminación:** Tira LED analógica o módulos LED de 5 canales (R, G, B, WW, CW).
3.  **Etapa de Potencia:** Transistores MOSFET (ej. IRLZ44N, TIP120) o un módulo controlador LED adecuado para manejar el amperaje de los LEDs.
4.  **Fuente de Alimentación:** Adecuada para el voltaje (usualmente 12V o 24V) y consumo de tu tira LED.

### 📌 Conexiones de Pines (Configuración por defecto)

| Canal LED | Pin ESP32 |
| :--- | :--- |
| **R** (Rojo) | `GPIO 17` |
| **G** (Verde) | `GPIO 16` |
| **B** (Azul) | `GPIO 5` |
| **WW** (Blanco Cálido) | `GPIO 19` |
| **CW** (Blanco Frío) | `GPIO 18` |

*(Nota: Los pines del ESP32 deben conectarse a las puertas (Gates) de los MOSFETs, no directamente a los LEDs).*

## 💻 Instalación y Configuración

1. Abre el archivo principal en el **Arduino IDE**.
2. Instala el soporte para la placa ESP32 en el Gestor de Placas si aún no lo has hecho.
3. **¡Importante! Modifica las siguientes líneas** en el código antes de compilar para adaptarlas a tu red:

```cpp
// --- CREDENCIALES WIFI ---
const char* ssid     = "TU_NOMBRE_DE_RED_WIFI";
const char* password = "TU_CONTRASEÑA";

// --- CONFIGURACIÓN IP ESTÁTICA ---
IPAddress local_IP(192, 168, 1, 38); // Cambia esto por la IP que desees asignarle
IPAddress gateway(192, 168, 1, 1);   // La IP de tu router
```

4. **Configura tu zona horaria** modificando el `gmtOffset_sec`. Por defecto está configurado para GMT+1 (3600 segundos). Si estás en otra zona, multiplícalo por 3600 (ej. GMT-5 sería `-18000`).

```cpp
const long gmtOffset_sec = 3600;      // GMT+1
```

5. Compila y sube el código a tu ESP32.

## 📱 Uso de la Interfaz Web

Una vez que el ESP32 arranque, se conectará a tu WiFi.
1. Abre un navegador web en tu móvil o PC (conectado a la misma red).
2. Escribe la dirección IP configurada (por defecto: `http://192.168.1.38`).
3. En la interfaz podrás:
   * Elegir la hora de la alarma y ver la confirmación de guardado.
   * Activar o desactivar el amanecer automático.
   * Apagar todo de emergencia con el botón rojo **STOP**.
   * Usar los deslizadores (Sliders) para controlar la luz a tu gusto en modo manual.

## ⚙️ Cómo funciona el Algoritmo del Amanecer

La secuencia `ejecutarSecuenciaAmanecer()` tiene una duración de **45 minutos** y se divide en varias fases mediante interpolación matemática:
* **Minutos 0-10:** Empieza con tonos rojizos muy suaves.
* **Minutos 10-20:** El rojo alcanza su pico, entra un naranja suave y el blanco cálido empieza a despertar.
* **Minutos 20-35:** Transición fuerte hacia amarillos y blancos cálidos.
* **Minutos 35-45:** Integración total del blanco frío (CW) para simular la luz plena del día.

## 📝 Licencia

Este proyecto es de código abierto. Puedes usarlo, modificarlo y distribuirlo libremente para tus proyectos personales de domótica.
