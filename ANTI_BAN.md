# АНТИ-БАН (Anti-Ban) — Полная документация

## Обзор защиты CounterFlame

Игра использует многослойную защиту от читов. Ниже перечислены все обнаруженные механизмы и способы их обхода.

---

## 1. ACTk (Anti-Cheat Toolkit)

### WallHackDetector (ESP детектор)
- **Класс:** `CodeStage.AntiCheat.Detectors.WallHackDetector`
- **Метод:** `StartDetection(Action callback, Vector3 spawnPosition)` — RVA `0x1234B94`
- **Что делает:** Сканирует кадр на предмет аномальных цветов (ESP боксы, ченасы через стены). При обнаружении вызывает коллбэк обнаружения.
- **Обход:** Хук на `StartDetection` — не вызывать оригинал (NOP). Детектор никогда не запустится.
- **Статус:** ✅ Реализован в `DetourWallHackDetector`

### ShootRaycastModule (Silent Aim детектор)
- **Класс:** `CodeStage.AntiCheat.Detectors.WallHackDetector` (вложенный метод)
- **Метод:** `ShootRaycastModule()` — RVA `0x1239F20`
- **Что делает:** Анализирует рейкасты выстрелов. Если вектор стрельбы резко отличается от вектора взгляда (кросхаир не движется, но пуля летит в цель) — помечает как чит.
- **Обход:** Хук на `ShootRaycastModule` — не вызывать оригинал.
- **Статус:** ✅ Реализован в `DetourShootRaycastModule`

---

## 2. ScarletWork.AntiCheat.RootDetect (Детект джейлбрейка/рута)

- **Класс:** `ScarletWork.AntiCheat.RootDetect`
- **Методы:**
  - `Start()` — RVA `0x140ED60` — Запуск проверки при старте сцены
  - `Checkroot()` — RVA `0x140EDF0` — Проверка наличия джейлбрейка/рута, возвращает bool
- **Что делает:** Проверяет наличие джейлбрейка iOS. При обнаружении — блокирует вход или бан.
- **Обход:** Хук на `Checkroot` — всегда возвращать `false`.
- **Статус:** ✅ Реализован в `DetourRootDetect_Checkroot`

```cpp
bool DetourRootDetect_Checkroot() {
    if (g_bypass_rootdetect) {
        return false;  // Античит не обнаруживает джейлбрейк
    }
    return ((RootDetect_Checkroot_t)g_real_rootdetect_checkroot)();
}
```

---

## 3. ApkHashChecker (Проверка целостности клиента)

- **Класс:** `ApkHashChecker`
- **Метод:** `.ctor()` — RVA `0x126F358`
- **Что делает:** При загрузке проверяет хеш бинарника клиента. Если клиент модифицирован (наш dylib инжекчен) — блокирует запуск.
- **Обход:** Хук на конструктор — сделать пустым (NOP). Проверка хеша не выполнится.
- **Статус:** ✅ Реализован в `DetourApkHashChecker_ctor`

```cpp
void DetourApkHashChecker_ctor() {
    // Ничего не делаем — обходим проверку хеша APK
}
```

---

## 4. ScarletWork.API.SessionGuard (Отзыв сессии)

- **Класс:** `ScarletWork.API.SessionGuard`
- **Поля:**
  - `static bool _isRevoked` — флаг отозванной сессии
  - `static string _reason` — причина отзыва
- **Методы:**
  - `Revoke(string reason)` — RVA `0x13F00CC` — Отзывает сессию игрока
  - `Reset()` — RVA `0x13F03D4` — Сброс состояния отзыва
- **Что делает:** При обнаружении чит-активности сервер может отправить команду отзыва сессии. Клиент вызывает `Revoke`, и аккаунт блокируется.
- **Обход:** Хук на `Revoke` — игнорировать вызов. Сессия никогда не отзывается клиентом.
- **Статус:** ✅ Реализован в `DetourSessionGuard_Revoke`

```cpp
void DetourSessionGuard_Revoke(const char* reason) {
    if (g_bypass_actk) {
        return;  // Игнорируем отзыв сессии
    }
    ((SessionGuard_Revoke_t)g_real_sessionguard_revoke)(reason);
}
```

---

## 5. ScarletWork.API.ScarletVersion (Проверка версии клиента)

- **Класс:** `ScarletWork.API.ScarletVersion`
- **Методы:**
  - `.cctor()` — RVA `0x13F044C` — Статический конструктор, запускает проверку версии
  - `GetVersion()` — RVA `0x13F00CC` — Возвращает текущую версию клиента
- **Что делает:** При запуске проверяет версию клиента на сервере. Если версия устарела или клиент модифицирован — блокирует вход.
- **Обход:** 
  1. Хук на `cctor` — не запускать проверку версии при старте
  2. Хук на `GetVersion` — всегда возвращать актуальную версию `"1.3.0.1"`
- **Статус:** ✅ Реализован в `DetourScarletVersion_cctor` и `DetourScarletVersion_GetVersion`

```cpp
const char* DetourScarletVersion_GetVersion() {
    if (g_bypass_version) {
        return "1.3.0.1";  // Всегда актуальная версия
    }
    return ((ScarletVersion_GetVersion_t)g_real_scarletversion_getversion)();
}
```

---

## 6. ScarletWork.API.ScarletEncryption (Шифрование запросов)

- **Класс:** `ScarletWork.API.ScarletEncryption`
- **Методы:**
  - `Encrypt(string name)` — RVA `0x13E5B58`
  - `Decrypt(string name)` — RVA `0x13E62AC`
- **Что делает:** Кастомное шифрование запросов к API. Ключ хранится в `.cctor` в рантайме.
- **Примечание:** Для полной работоспособности чита нужно либо подписывать запросы правильным ключом, либо отключать проверку подписи на сервере (невозможно без доступа к серверу). В текущей реализации шифрование не трогается — если игра требует подписи, чит будет работать, но запросы могут отвергаться сервером.
- **Статус:** ⚠️ Требует отдельного анализа (не реализовано в текущей версии)

---

## 7. HMAC-подпись запросов

- **Класс:** `ScarletWork.API.Client`
- **Поля:**
  - `private static readonly string[] _hmacParts` — части HMAC-ключа
  - `private static string _accessToken` — токен доступа
  - `private static string _refreshToken` — токен обновления
- **Что делает:** Каждый запрос к API подписывается HMAC-ключом. Сервер проверяет подпись.
- **Обход:** Требует извлечения ключа из `.cctor` класса `Client` и правильного подписывания всех исходящих запросов. В текущей версии не реализовано.
- **Статус:** ⚠️ Требует Reverse Engineering ключа подписи

---

## 8. SystemInfo.deviceUniqueIdentifier (HWID)

- **Метод:** `UnityEngine.SystemInfo.get_deviceUniqueIdentifier` — RVA `0x2BB19D8`
- **Что делает:** Возвращает уникальный идентификатор устройства. Сервер может забанить по HWID.
- **Обход:** Хук на метод — возвращать поддельный HWID `SPOOFED_XXXXXXXX`.
- **Статус:** ✅ Реализован в `DetourGetDeviceUniqueIdentifier`

---

## Таблица всех оффсетов антибана

| Метод | Namespace | RVA (Offset) | VA | Обход |
|-------|-----------|--------------|-----|-------|
| `FirePellets` | (без неймспейса) | `0x12783E4` | `0x12783E4` | Silent Aim (чит) |
| `PlayerDamage` | (без неймспейса) | `0x12796AC` | `0x12796AC` | Попадание (чит) |
| `AddVisible` | (без неймспейса) | `0x15C9670` | `0x15C9670` | ESP список игроков |
| `WallHackDetector.StartDetection` | `CodeStage.AntiCheat.Detectors` | `0x1234B94` | `0x1234B94` | **ОБХОД** — не запускать детектор |
| `ShootRaycastModule` | `CodeStage.AntiCheat.Detectors` | `0x1239F20` | `0x1239F20` | **ОБХОД** — не детектить рейкасты |
| `RootDetect.Start` | `ScarletWork.AntiCheat` | `0x140ED60` | `0x140ED60` | Не запускать проверку |
| `RootDetect.Checkroot` | `ScarletWork.AntiCheat` | `0x140EDF0` | `0x140EDF0` | **ОБХОД** — всегда false |
| `ApkHashChecker.ctor` | (без неймспейса) | `0x126F358` | `0x126F358` | **ОБХОД** — пустой конструктор |
| `SessionGuard.Revoke` | `ScarletWork.API` | `0x13F00CC` | `0x13F00CC` | **ОБХОД** — не отзывать сессию |
| `SessionGuard.Reset` | `ScarletWork.API` | `0x13F03D4` | `0x13F03D4` | **ОБХОД** — не сбрасывать сессию |
| `ScarletVersion.cctor` | `ScarletWork.API` | `0x13F044C` | `0x13F044C` | **ОБХОД** — не проверять версию |
| `ScarletVersion.GetVersion` | `ScarletWork.API` | `0x13F00CC` | `0x13F00CC` | **ОБХОД** — возвращать "1.3.0.1" |
| `SystemInfo.deviceUniqueIdentifier` | `UnityEngine` | `0x2BB19D8` | `0x2BB19D8` | **ОБХОД** — HWID споффинг |

---

## Меню настроек антибана

В чит-меню есть вкладка **Anti-Ban** с переключателями:

| Настройка | Описание | По умолчанию |
|-----------|----------|--------------|
| HWID Spoof | Подмена идентификатора устройства | ✅ Вкл |
| Bypass ACTk | Обход ACTk (WallHack, ShootRaycast) | ✅ Вкл |
| Bypass Version | Обход проверки версии клиента | ✅ Вкл |
| Bypass RootDetect | Обход детекта джейлбрейка | ✅ Вкл |

---

## Установка и тестирование

1. Собрать `.dylib` через GitHub Actions (workflow `Build CounterFlame Cheat`)
2. Инжектить в IPA через [CodeVN Inject](https://ipatool.codevn.net/inject-dylib/)
3. Установить на iPhone через Sideloadly
4. Запустить игру, открыть меню чита (кнопка INSERT)
5. Проверить вкладку Anti-Ban — все переключатели должны быть включены

---

## Ограничения

1. **HMAC-подпись запросов** — не обходится в текущей версии. Если сервер требует подписи каждого запроса, чит может работать, но некоторые запросы (вход, покупки) могут отвергаться. Требуется извлечение ключа `_hmacParts` из `.cctor` класса `Client`.
2. **Телеметрия Firebase** — игра отправляет логи на сервер Firebase. В текущей версии не блокируется. Можно добавить хук на отправку логов.
3. **Серверная проверка** — если сервер ведет слепой учет аномалий (аномально высокая точность, невозможные хитбоксы), бан может прийти с задержкой. Рекомендуется не использовать чит на основном аккаунте.
