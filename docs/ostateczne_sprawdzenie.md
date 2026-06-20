# Ostateczne sprawdzenie MatrixHub - jeden duzy goal z podgolami

Ten plik jest gotowym promptem/instrukcja dla chatu/agenta uruchomionego w trybie goal.
Wklej calosc jako jeden goal. Agent ma pracowac autonomicznie, modyfikowac kod,
testowac na hoscie i na urzadzeniu, commitowac po kazdym zakonczonym podgolu i
przechodzic do kolejnego punktu bez pytania uzytkownika, chyba ze dalsza praca jest
technicznie niemozliwa bez zewnetrznej informacji.

## Glowne zadanie

Doprowadz projekt MatrixHub do poziomu commercial grade/golden quality.

Kod obecnie dziala, ale projekt ma bardzo duzo kombinacji funkcji i malo twardych
smoke testow end-to-end. Celem nie jest kosmetyka. Celem jest pelny audyt,
poprawa, instrumentacja runtime, stabilnosc, mierzalnosc, przewidywalnosc,
uzytecznosc i kontrakt backend/frontend/SDK.

Zakres obejmuje CALOSC:

- firmware C++ dla ESP32-S3,
- lokalny framework `lib/framework`,
- API HTTPS/JWT,
- WebSockety,
- frontend SvelteKit w `interface/`,
- TypeScript SDK w `packages/device-sdk`,
- skrypty diagnostyczne i testowe,
- dokumentacje techniczna tam, gdzie jest potrzebna do utrzymania standardu,
- testy hostowe, frontendowe, E2E i testy na realnym urzadzeniu.

## Dane operacyjne

Urzadzenie:

- stale IP: `192.168.0.30`
- mDNS: `plantcare.local`
- tylko HTTPS, brak HTTP
- podstawowy URL: `https://192.168.0.30`
- alternatywny URL: `https://plantcare.local`
- certyfikat moze byc self-signed, uzywaj `curl -k` oraz `requests.verify = False`
- login JWT: `admin`
- haslo JWT: `admin`
- haslo sudo hosta, jesli naprawde potrzebne do instalacji zaleznosci: `test`

Build:

- aktywne srodowisko firmware: `waveshare_esp32s3_matrix`
- w `platformio.ini` jest tylko jeden docelowy env urzadzenia plus env natywny
- build firmware: `/home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix`
- szybki build bez UI: `SKIP_UI=1 /home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix`
- unit testy hostowe: `/home/test/.platformio/penv/bin/pio test -e native`
- frontend: komendy z katalogu `interface/`

Wazne: po kazdym podgolu wykonaj commit, ale nigdy nie rob push.

Minimalny rytual zamkniecia kazdego podgolu:

```bash
git status --short
git diff --check
/home/test/.platformio/penv/bin/pio test -e native
cd interface && npm run check
cd interface && npm run test:run
git status --short
git add <tylko-pliki-dotkniete-w-tym-podgolu>
git commit -m "<krotki, konkretny opis podgolu>"
git status --short
```

Jezeli podgol jest firmware-only i frontend nie byl dotkniety, frontend checks moga
byc pominiete tylko wtedy, gdy w notatce do commita zapiszesz dlaczego. Jezeli
podgol dotyka kontraktu API, endpointow, danych websocketowych albo UI, frontend i
SDK musza byc testowane.

## Zasady pracy autonomicznej

1. Najpierw czytaj kod, potem poprawiaj.
2. Nie pytaj uzytkownika o oczywiste rzeczy. Rob rozsadne zalozenia i dokumentuj je
   w commit message albo w pliku diagnostycznym.
3. Jezeli test wymaga zainstalowania zaleznosci, zainstaluj je po uzyskaniu zgody
   narzedzia/escalation. Nie obchodz sandboxa.
4. Nie niszcz danych uzytkownika. Nie rob `git reset --hard`, nie usuwaj
   niepowiazanych zmian, nie czysc konfiguracji urzadzenia bez kopii.
5. Przed restartem/flashowaniem urzadzenia zrob snapshot aktualnych ustawien przez
   endpointy i zapisz go w katalogu roboczym diagnostyki, najlepiej
   `artifacts/diagnostics/<timestamp>/`. Nie commituj duzych logow ani sekretow.
6. Po kazdej zmianie, ktora moze wymagac restartu runtime lub urzadzenia, wykonaj
   restart kontrolowany i poczekaj az `https://192.168.0.30` wroci online.
7. Wszystkie nowe endpointy diagnostyczne musza byc chronione JWT, preferencyjnie
   admin-only. Nie wystawiaj sekretow, tokenow, hasel, prywatnych kluczy ani pelnych
   payloadow z credentialami.
8. Kazdy podgol ma miec jednoznaczne kryterium akceptacji: test hostowy, test
   endpointow, test WebSocket, test UI, test restartu albo pomiar runtime.
9. Jezeli znajdziesz bug krytyczny po drodze, napraw go w ramach aktualnego podgolu
   albo stworz natychmiastowy podgol naprawczy, przetestuj i commituj.
10. Jezeli jakis test jest niestabilny, nie ignoruj go. Zrob albo naprawe, albo
    osobny commit stabilizujacy test/infrastrukture.

## Wspolne komendy i wzorce

Logowanie JWT przez curl:

```bash
TOKEN="$(curl -sk https://192.168.0.30/rest/signIn \
  -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"admin"}' \
  | jq -r '.access_token')"
test -n "$TOKEN" && test "$TOKEN" != "null"
```

Jezeli `jq` nie jest dostepny, uzyj Pythona/Node albo dodaj zaleznosc w sposob
kontrolowany. Nie zapisuj tokenu do commita.

Przykladowe odpytanie endpointu:

```bash
curl -sk https://192.168.0.30/api/system/info \
  -H "Authorization: Bearer $TOKEN"
```

Sprawdzenie obu adresow:

```bash
curl -skI https://192.168.0.30/
curl -skI https://plantcare.local/
```

E2E frontend przeciwko realnemu urzadzeniu:

```bash
cd interface
DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e
```

Soak test docelowo ma uzywac HTTPS:

```bash
DEVICE_URL=https://192.168.0.30 DEVICE_USER=admin DEVICE_PASSWORD=admin \
  python scripts/tests/soak_test.py --duration 30m --interval 10s
```

## Mapa funkcjonalnosci, ktora musi zostac pokryta

Firmware i runtime:

- boot, inicjalizacja, `Application`, `ServiceRegistry`, `InitSequence`,
- HTTPS server `PsychicHttpsServer` i lokalny `ESP32SvelteKit`,
- JWT auth, role, rate limiting, sign-in, verify authorization,
- WiFi STA/AP, mDNS, NTP, restart, sleep, factory reset,
- power management, deep sleep, wake, hygiene sleep, activity tracking,
- watchdog, shutdown sequence, runtime restart, coredump,
- heap/PSRAM/internal RAM, stack high watermark, task list,
- logging: serial, ring buffer, live tail, log files, poziomy logowania,
- filesystem LittleFS, file manager, upload/download/remove/list,
- binary sensor data logger i charts,
- SCD41, IMU, compensation,
- matrix LED, matrix manager, layers, alarm icons, custom icons, menu,
- alarms: rules, evaluator, cooldowns, status, notifier,
- notifications: Telegram, Telegram commands, Webhook, Pushover, workers,
- Shelly discovery/config/control/worker,
- BLE scanner, BLE settings, discovery, cache, whitelist, parsers,
- WiFi sensing RSSI i CSI streaming,
- AirMouse, keyboard, mouse jiggler, USB terminal,
- macros engine, parser, repository, runtime, boot scripts,
- UDP push.

API i kontrakt:

- framework REST: `/rest/signIn`, `/rest/verifyAuthorization`,
  `/rest/securitySettings`, `/rest/features`, `/rest/wifiStatus`,
  `/rest/wifiSettings`, `/rest/scanNetworks`, `/rest/listNetworks`,
  `/rest/apStatus`, `/rest/apSettings`, `/rest/ntpStatus`,
  `/rest/ntpSettings`, `/rest/time`, `/rest/restart`, `/rest/factoryReset`,
  `/rest/sleep`,
- custom REST: `/api/system/info`, `/api/system/tasks`,
  `/api/system/network`, `/api/system/wifi/recover`, `/api/config`,
  `/api/matrix/settings`, `/api/alarms/rules`, `/api/ble/status`,
  `/api/ble/settings`, `/api/ble/scan`, `/api/wifisensing/config`,
  `/api/shelly/devices`, `/api/shelly/control`, `/api/heartbeat`,
  `/api/heartbeat/test`, `/api/udp`, `/api/udp/test`,
  `/api/notifications/settings`,
  `/api/notifications/telegram/test`,
  `/api/notifications/webhook/test`,
  `/api/notifications/pushover/test`,
  `/api/airmouse/status`, `/api/airmouse/config`,
  `/api/airmouse/calibrate`, `/api/keyboard/config`,
  `/api/keyboard/type`, `/api/keyboard/press`,
  `/api/macros/settings`, `/api/macros`, `/api/macros/run`,
  `/api/macros/stop`, `/api/macros/status`, `/api/macros/content`,
  `/api/macros/delete`, `/api/compensation`, `/api/usbterminal/config`,
  `/api/logs`, `/api/logs/download`, `/api/logs/delete`,
  `/rest/logs/tail`, `/rest/fs/list`, `/rest/fs/download`,
  `/rest/fs/remove`, `/rest/fs/upload`,
- WebSocket: `/ws/system`, `/ws/csi`, `/ws/usbterminal`,
- frontend TS clients in `interface/src/lib/services/api/**`,
- frontend stores/parsers in `interface/src/lib/stores/system/**`,
- TS SDK in `packages/device-sdk/src/**`,
- docs and screenshots only after real UI behavior is correct.

## Podgol 0 - baseline, inwentaryzacja i stan startowy

Cel:
Zrobic pelny obraz stanu repo, testow i urzadzenia przed zmianami.

Kroki:

1. Sprawdz `git status --short`. Nie ruszaj niepowiazanych zmian.
2. Zbierz liste modulow: `rg --files src interface/src packages/device-sdk scripts test`.
3. Zbierz liste endpointow z C++ i TS klientow:
   - `rg -n "_server->on\\(|HttpEndpoint<|/api/|/rest/|/ws/" src lib/framework interface/src packages/device-sdk`
4. Zaloguj sie do urzadzenia przez HTTPS i zapisz baseline odpowiedzi:
   - `/api/system/info`
   - `/api/system/tasks?details=1`
   - `/api/system/network`
   - `/api/config`
   - `/rest/power/status`
   - `/api/ble/status`
   - `/api/alarms/rules?includeStatus=1`
   - `/api/matrix/settings`
   - `/api/wifisensing/config`
   - `/api/notifications/settings`
   - `/api/macros/status`
   - `/api/logs`
5. Zapisz baseline w `artifacts/diagnostics/<timestamp>/baseline/`.
6. Uruchom aktualne testy hostowe i frontendowe.
7. Uruchom szybki endpoint smoke przeciwko realnemu urzadzeniu.

Kryteria akceptacji:

- wiadomo, ktore testy przechodza przed zmianami,
- wiadomo, ktore endpointy sa dostepne,
- znane sa aktualne wartosci heap, PSRAM, stack HWM, task count, uptime,
- znane sa wszystkie odchylenia/stare skrypty HTTP, ktore trzeba poprawic.

Commit:

Jesli powstaly tylko lokalne artefakty niecommitowane, commit nie jest wymagany.
Jesli dodasz baseline tooling albo poprawisz skrypty pomocnicze, commit:

```bash
git add scripts docs ostateczne_sprawdzenie.md
git commit -m "Add baseline diagnostics workflow"
```

## Podgol 1 - ujednolicenie skryptow diagnostycznych pod HTTPS/JWT

Cel:
Wszystkie skrypty diagnostyczne i testowe maja dzialac z urzadzeniem
`https://192.168.0.30`, JWT `admin/admin`, self-signed TLS i zmiennymi
srodowiskowymi.

Zakres:

- `scripts/tests/stress_test.py`
- `scripts/tests/soak_test.py`
- `scripts/tests/feature_toggle.py`
- `scripts/tests/verify_persistence.py`
- `scripts/tests/verify_security.py`
- `scripts/diagnostics/check_memory.py`
- `scripts/diagnostics/check_tasks.py`
- `scripts/diagnostics/check_config.py`
- `scripts/diagnostics/check_ble_config.py`
- `scripts/diagnostics/check_heartbeat.py`
- `scripts/diagnostics/check_udp.py`
- `scripts/diagnostics/check_shelly.py`
- `scripts/diagnostics/trigger_*`
- `tools/csi_client.py`
- `tools/ble_test_client.py`, jezeli uzywa hosta/API.

Wymagania:

- domyslny `DEVICE_URL` ma byc `https://192.168.0.30`,
- opcjonalny alias `https://plantcare.local`,
- brak hardcoded starych adresow testowych,
- `requests.Session()` z retry i `verify=False` przy HTTPS,
- centralny helper loginu i ponowienia po 401,
- brak interaktywnych promptow,
- wyjscie != 0 przy realnym failure,
- JSON output albo CSV output tam, gdzie to sluzy automatyzacji,
- endpointy w skryptach musza odpowiadac aktualnemu kodowi.

Testy:

```bash
python scripts/tests/stress_test.py --cycles 1 --delay 0.05
python scripts/diagnostics/check_memory.py
python scripts/diagnostics/check_tasks.py
python scripts/diagnostics/check_config.py
python scripts/tests/verify_security.py
```

Kryteria akceptacji:

- skrypty nie probuja HTTP,
- poprawnie loguja sie JWT,
- dzialaja z self-signed HTTPS,
- daja jednoznaczny pass/fail,
- nie wymagaja recznej edycji IP.

Commit:

```bash
git status --short
/home/test/.platformio/penv/bin/pio test -e native
git add scripts tools docs
git commit -m "Unify device diagnostics over HTTPS"
```

## Podgol 2 - manifest kontraktu API/backend/frontend/SDK

Cel:
Nie moze byc ukrytego rozjazdu miedzy endpointami C++, frontendem i SDK.

Kroki:

1. Stworz lub zaktualizuj dokument/manifest kontraktu, np.
   `docs/engineering/api-contract.md` albo generowany JSON w `docs/engineering/api-contract.json`.
2. Manifest ma zawierac:
   - metoda HTTP,
   - path,
   - auth: public/auth/admin,
   - request body schema high-level,
   - response body schema high-level,
   - czy endpoint wymaga restartu po zmianie,
   - ktory frontend service go uzywa,
   - czy jest pokryty testem hostowym, frontendowym, E2E, device smoke.
3. Porownaj:
   - `src/api/**`,
   - `lib/framework/**`,
   - `interface/src/lib/services/api/**`,
   - `interface/src/lib/stores/**`,
   - `packages/device-sdk/src/**`,
   - `docs/user-guide/**`.
4. Dodaj test lub skrypt, ktory wykrywa najprostsze rozjazdy pathow.

Wymagane naprawy:

- TS client nie moze wolac endpointu, ktory w firmware nie istnieje.
- Firmware endpoint nie moze miec innego auth/shape niz oczekuje frontend.
- SDK nie moze zostac z tylu dla najwazniejszych endpointow system/ble/matrix/shelly.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native
cd interface && npm run check
cd interface && npm run test:run
cd interface && DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e
```

Kryteria akceptacji:

- istnieje aktualny kontrakt,
- endpoint smoke uzywa tego samego zrodla prawdy albo jest z nim zgodny,
- frontend i SDK sa zsynchronizowane z firmware.

Commit:

```bash
git add docs scripts interface packages src lib test
git commit -m "Document and verify API contract"
```

## Podgol 3 - diagnostyczne endpointy runtime commercial grade

Cel:
Runtime ma byc mierzalny bez zgadywania z serial monitora. Jezeli obecne
`/api/system/info`, `/api/system/tasks` i `/api/system/network` nie wystarczaja,
dodaj admin-only endpointy diagnostyczne.

Preferowany ksztalt:

- `/api/diagnostics/summary`
- `/api/diagnostics/heap`
- `/api/diagnostics/tasks`
- `/api/diagnostics/mutexes`
- `/api/diagnostics/endpoints`
- `/api/diagnostics/features`

Jezeli lepiej pasuje do architektury, rozszerz istniejace `/api/system/*`, ale
zachowaj kontrakt i testy.

Minimalne pola runtime:

- uptime,
- boot reason, reset reason, last shutdown reason,
- app version, build flags istotne dla BLE/USB/CSI/HTTPS,
- free heap, min free heap, largest heap block,
- internal RAM free/largest/min,
- PSRAM free/largest/min,
- heap fragmentation proxy,
- task count,
- task name, core, priority, state, stack high watermark,
- watchdog status i registered tasks, jezeli dostepne,
- HTTP active clients, max clients, forced WS removals, queue drops, heap fallbacks,
- WS system clients, CSI clients, USB terminal clients,
- BLE running/scanner active/discovery active/cache size,
- CSI service state, consumers, queue drops,
- UDP worker state,
- notification worker states,
- alarm rules count, active triggered count,
- matrix task state and frame/update counters,
- mutex contention counters albo lock timeout counters dla krytycznych modulow,
- endpoint request counts/errors/latency buckets, jezeli mozliwe bez duzego narzutu.

Wymagania:

- endpointy admin-only,
- streaming JSON tam, gdzie payload moze rosnac,
- brak alokowania duzych DOM JSON w hot path,
- brak sekretow,
- testy native dla serializerow,
- frontend system status moze pokazywac te dane, ale UI nie jest wymagany w tym
  podgolu, chyba ze obecne UI juz probuje je czytac.

Testy:

```bash
curl -sk https://192.168.0.30/api/diagnostics/summary -H "Authorization: Bearer $TOKEN"
curl -sk https://192.168.0.30/api/diagnostics/heap -H "Authorization: Bearer $TOKEN"
curl -sk https://192.168.0.30/api/diagnostics/tasks -H "Authorization: Bearer $TOKEN"
curl -sk https://192.168.0.30/api/diagnostics/mutexes -H "Authorization: Bearer $TOKEN"
/home/test/.platformio/penv/bin/pio test -e native
```

Kryteria akceptacji:

- stan runtime jest mierzalny z endpointow,
- dane pozwalaja wykryc memory leak, stack erosion, mutex contention i WS drops,
- skrypty soak/stress potrafia te endpointy wykorzystac.

Commit:

```bash
git add src test scripts docs interface packages
git commit -m "Add runtime diagnostics endpoints"
```

## Podgol 4 - bounded JSON, payload limits i streaming hot path

Cel:
Usunac falszywe poczucie limitow JSON i upewnic sie, ze parsery/serializery maja
realne limity pamieci.

Kontekst:

Wczesniejsze notatki wskazuja ryzyko w `SpiRamJsonDocument(capacity)` i
`BaseApiService::parseJsonBody(docSize)`, jezeli ArduinoJson v7 ignoruje
pojemnosc w sposob, ktory nie daje realnego limitu alokacji. Trzeba to
zweryfikowac na aktualnym kodzie, nie zakladac w ciemno.

Kroki:

1. Sprawdz `src/system/memory/PsramAllocator.h`, `BaseApiService.h` i wszystkie
   endpointy POST.
2. Napisz test natywny, ktory potwierdza rzeczywiste zachowanie limitow:
   - za duzy body daje 413,
   - invalid JSON daje 400,
   - JSON przekraczajacy limit nie powoduje wzrostu heap bez limitu,
   - endpoint nie akceptuje nadmiarowych pol, jezeli schema tego wymaga.
3. Dla duzych odpowiedzi preferuj `JsonResponseWriter`/streaming zamiast
   `DynamicJsonDocument + measureJson + serializeJson`.
4. Przejrzyj:
   - `/api/system/*`,
   - `/ws/system` snapshoty,
   - `/api/alarms/rules`,
   - `/api/logs`,
   - `/api/macros`,
   - notification tests,
   - file manager.
5. Dodaj wspolne helpery dla limitow i bledow, jezeli obecne sa niespojne.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_http_endpoint_transaction"
/home/test/.platformio/penv/bin/pio test -e native -f "test_json_utils"
/home/test/.platformio/penv/bin/pio test -e native
python scripts/tests/verify_security.py
python scripts/tests/stress_test.py --cycles 5 --delay 0.02
```

Kryteria akceptacji:

- kazdy publiczny/admin POST ma jawny limit payloadu,
- bledy sa spojne: 400 invalid JSON, 401/403 auth, 413 payload too large, 503 busy,
- hot path nie buduje duzych JSON DOM bez powodu,
- testy potwierdzaja zachowanie.

Commit:

```bash
git add src test scripts docs
git commit -m "Harden bounded JSON handling"
```

## Podgol 5 - mutexy, locki, deadlocki i contention

Cel:
Nie ma deadlockow, blokad bez timeoutu w miejscach ryzykownych, ani lockow
trzymanych podczas I/O/network/duzych alokacji.

Kroki:

1. Zmapuj wszystkie:
   - `SemaphoreHandle_t`,
   - `xSemaphoreTake`,
   - `ScopeLock`,
   - `RecursiveScopeLock`,
   - `portMAX_DELAY`,
   - kolejki i cleanup semaphores.
2. Podziel locki na:
   - hot path,
   - config/control plane,
   - shutdown/restart,
   - filesystem,
   - network,
   - websocket,
   - BLE,
   - RTC config.
3. Sprawdz szczegolnie:
   - `UdpPusher`,
   - `WsTaskQueue`, `WsPayloadPool`, `WsClientManager`, `ChannelSubscriptions`,
   - `RtcConfigStore`, `RtcStatefulService`,
   - `PowerSettingsService`,
   - `MatrixManagerService`, `MatrixLayerManager`, `MatrixTask`,
   - `BleScannerCache`, `BleScannerLifecycle`,
   - `WifiSensingTaskRunner`,
   - `USB_TERMINAL`,
   - alarm manager lock,
   - file manager FS mutex.
4. Dodaj lekkie liczniki lock timeout/slow lock w runtime diagnostics.
5. Zmien `portMAX_DELAY` w hot/control path na jawne timeouty tam, gdzie to
   bezpieczne. W shutdown moze zostac dluzszy timeout, ale musi byc uzasadniony.
6. Upewnij sie, ze lock nie obejmuje:
   - HTTP send,
   - WebSocket send,
   - filesystem streaming,
   - zewnetrznego network call,
   - dlugiego JSON serialization,
   - `vTaskDelay`,
   - restart/sleep.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native
python scripts/tests/stress_test.py --cycles 20 --delay 0.01
python scripts/tests/soak_test.py --duration 30m --interval 5s
curl -sk https://192.168.0.30/api/diagnostics/mutexes -H "Authorization: Bearer $TOKEN"
```

Kryteria akceptacji:

- zero deadlockow w stress/soak,
- lock timeout counters nie rosna w normalnym smoke,
- hot path ma bounded wait,
- wszystkie retry/skip sa logowane z throttlingiem, bez spamowania.

Commit:

```bash
git add src test scripts docs
git commit -m "Audit and harden runtime locking"
```

## Podgol 6 - pamiec runtime, heap drift i stack budget

Cel:
Zuzycie pamieci jest stabilne, przewidywalne i mierzalne podczas normalnej pracy
oraz podczas wlaczania wszystkich funkcji.

Kroki:

1. Upewnij sie, ze diagnostyka pokazuje:
   - internal heap free/min/largest,
   - PSRAM free/min/largest,
   - total heap free/min/largest,
   - task stack HWM,
   - alokacje/fallbacki WS,
   - queue drops,
   - najgorszy task po HWM.
2. Zrob baseline przy funkcjach domyslnych.
3. Wlacz i przetestuj po kolei:
   - BLE,
   - WiFi sensing RSSI,
   - CSI streaming,
   - keyboard,
   - airmouse,
   - jiggler,
   - macros,
   - USB terminal,
   - UDP,
   - heartbeat,
   - Telegram commands, jesli skonfigurowane credentiale istnieja,
   - Webhook/Pushover tylko z bezpiecznymi test endpoints albo mockami.
4. Po kazdym wlaczeniu zrob snapshot heap/tasks i restart, jezeli wymaga tego
   kontrakt funkcji.
5. Uruchom 30-60 min soak przy maksymalnym sensownym zestawie funkcji.
6. Ustal progi:
   - brak resetow/WDT,
   - brak monotonicznego spadku internal free heap,
   - min stack watermark dla kazdego taska ma miec bezpieczny margines,
   - brak rosnacych WS queue drops w idle,
   - brak fragmentacji, ktora psuje najwiekszy blok alokacji.
7. Jezeli stack jest za duzy, zmniejsz go tylko po pomiarze. Jezeli za maly,
   zwieksz i udokumentuj.

Testy:

```bash
python scripts/tests/soak_test.py --duration 1h --interval 10s
python scripts/tests/stress_test.py --cycles 50 --delay 0.02
curl -sk https://192.168.0.30/api/system/tasks?details=1 -H "Authorization: Bearer $TOKEN"
curl -sk https://192.168.0.30/api/diagnostics/heap -H "Authorization: Bearer $TOKEN"
```

Kryteria akceptacji:

- powstaje raport pamieci w docs albo artifacts,
- stack budget jest uzasadniony pomiarami,
- drift pamieci jest stabilny albo znalezione przyczyny sa naprawione,
- system przechodzi 1h soak bez resetu i bez krytycznych spadkow.

Commit:

```bash
git add src test scripts docs
git commit -m "Measure and stabilize runtime memory budgets"
```

## Podgol 7 - logowanie serial/live tail/log files

Cel:
Logi maja byc jednolite, przydatne diagnostycznie, nietoksyczne dla wydajnosci i
bezpieczne dla sekretow.

Kroki:

1. Przejrzyj `src/system/logging/**`, `LogRingBuffer`, `LogOutput`,
   `/rest/logs/tail`, `/api/logs`, serial monitor.
2. Ujednolic tagi logow:
   - krotkie,
   - stabilne,
   - zgodne z modulem,
   - bez losowego miksu `ESP_LOG*`, `LOG*`, `Serial.print`, chyba ze swiadomie.
3. Ustal poziomy:
   - ERROR: realny blad wymagajacy uwagi,
   - WARN: degradacja/timeout/retry,
   - INFO: lifecycle, restart, zmiana configu,
   - DEBUG: diagnostyka rozwoju,
   - VERBOSE: bardzo szczegolowe.
4. Dodaj throttling tam, gdzie log moze spamowac:
   - BLE adv,
   - WiFi disconnect/retry,
   - CSI,
   - WS drops,
   - lock timeout repeated,
   - sensor read failures.
5. Upewnij sie, ze sekrety sa redacted:
   - JWT,
   - hasla,
   - bot token,
   - webhook URL z tokenem,
   - Pushover keys,
   - WiFi password.
6. Popraw `/api/config` log level tak, zeby zmiana poziomu dzialala runtime i po
   restarcie.
7. Zrob serial monitor capture podczas bootu i smoke. Sprawdz, czy logi mowia:
   - build/app version,
   - HTTPS start,
   - mDNS,
   - service init,
   - memory baseline,
   - restart reason,
   - failed optional features.

Testy:

```bash
curl -sk https://192.168.0.30/api/config -H "Authorization: Bearer $TOKEN"
curl -sk https://192.168.0.30/rest/logs/tail?lines=200 -H "Authorization: Bearer $TOKEN"
curl -sk https://192.168.0.30/api/logs -H "Authorization: Bearer $TOKEN"
/home/test/.platformio/penv/bin/pio test -e native -f "test_log_ring_buffer"
```

Kryteria akceptacji:

- logi sa czytelne,
- brak sekretow,
- brak spamowania w normalnym 10-min smoke,
- live tail i pliki logow dzialaja po HTTPS.

Commit:

```bash
git add src test docs interface scripts
git commit -m "Normalize runtime logging diagnostics"
```

## Podgol 8 - security hardening HTTPS/JWT/rate limit/input

Cel:
Security ma byc gotowe na commercial grade przy zalozeniu dev credentials na tym
konkretnym urzadzeniu.

Kroki:

1. Przejrzyj:
   - `lib/framework/security/**`,
   - JWT service,
   - auth wrappers,
   - request authorizer,
   - rate limiter,
   - sign-in async worker,
   - static security headers,
   - CORS/headers,
   - default credentials warning,
   - production TODO w `factory_settings.ini`.
2. Testuj:
   - brak tokenu -> 401,
   - user bez admina -> 403 dla admin routes,
   - invalid token -> 401,
   - revoked/old token po zmianie hasla -> odrzucony,
   - payload > limit -> 413,
   - invalid JSON -> 400,
   - path traversal file manager -> odrzucony,
   - upload do chronionych sciezek logow -> odrzucony,
   - rate limit loginu -> 429 albo rownowazna ochrona,
   - slowloris/timeout -> polaczenie zamkniete.
3. Upewnij sie, ze nowe diagnostic endpoints sa admin-only.
4. Popraw frontend komunikaty bledow auth/input tak, zeby byly uzyteczne.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_jwt_authenticator"
/home/test/.platformio/penv/bin/pio test -e native -f "test_request_authorizer"
/home/test/.platformio/penv/bin/pio test -e native -f "test_rate_limiter"
/home/test/.platformio/penv/bin/pio test -e native -f "test_security_settings"
python scripts/tests/verify_security.py
cd interface && npm run test:run
```

Kryteria akceptacji:

- security tests przechodza,
- device security smoke przechodzi,
- brak publicznego endpointu z runtime diagnostics,
- frontend poprawnie obsluguje 401/403/session expiry.

Commit:

```bash
git add src lib test scripts interface docs
git commit -m "Harden HTTPS JWT security flows"
```

## Podgol 9 - boot, restart, shutdown, coredump, watchdog

Cel:
Restart i shutdown sa przewidywalne, nie zostawiaja wiszacych taskow, nie wywoluja
false watchdogow i daja diagnostyke po awarii.

Kroki:

1. Przejrzyj:
   - `Application`,
   - `ApplicationRuntime`,
   - `InitSequence`,
   - `ServiceRegistryLifecycle`,
   - `ShutdownSequence`,
   - `RuntimeRestart`,
   - `TaskWatchdog`,
   - `BootTracker`,
   - `FactoryResetHook`, `RestartHook`,
   - `decode_coredump.py`.
2. Testuj restart:
   - `/rest/restart`,
   - restart po zmianie BLE/keyboard/USB terminal jesli wymagany,
   - restart po WiFi settings,
   - restart po macro boot script setting,
   - restart po awarii symulowanej tylko jesli jest bezpieczny test hook.
3. Weryfikuj po kazdym restarcie:
   - `uptime` resetuje sie,
   - last boot reason jest poprawny,
   - config przetrwal,
   - taski nie sa zdublowane,
   - WS dziala po reconnect,
   - BLE/CSI/USB tasks nie zostaly w stanie polowicznym.
4. Dodaj brakujace testy native dla shutdown/restart state machine.
5. Decode coredump smoke: skrypt nie musi znalezc coredump, ale ma dawac
   czytelny wynik.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_runtime_restart"
/home/test/.platformio/penv/bin/pio test -e native -f "test_boot_tracker"
curl -sk -X POST https://192.168.0.30/rest/restart -H "Authorization: Bearer $TOKEN"
python scripts/diagnostics/decode_coredump.py
```

Kryteria akceptacji:

- 10 restartow pod rzad bez manualnej interwencji,
- brak WDT panic w logach,
- po restarcie endpointy i WS wracaja,
- task count nie dryfuje.

Commit:

```bash
git add src lib test scripts docs
git commit -m "Stabilize restart and watchdog diagnostics"
```

## Podgol 10 - system API, WebSocket `/ws/system` i snapshoty

Cel:
System status, taski, network, WS snapshots i channel subscriptions sa lekkie,
spojne i odporne na klientow frontendowych.

Kroki:

1. Przejrzyj:
   - `SystemApiService`,
   - `SystemWebsocketBroadcaster`,
   - `SystemSnapshotTransport`,
   - `System*Snapshots.cpp`,
   - `ChannelSubscriptions`,
   - `WsEndpointRuntime`,
   - frontend parsers w `interface/src/lib/stores/system/**`,
   - SDK `packages/device-sdk/src/ws.ts`.
2. Ogranicz podwojne serializacje JSON.
3. Weryfikuj oversize frame handling.
4. Testuj:
   - subscribe/unsubscribe,
   - snapshot request,
   - binary packet parsers,
   - reconnect po restart,
   - wiele klientow,
   - zamkniecie klienta,
   - queue drops.
5. Dodaj testy dla parserow i C++ serialization tam, gdzie brakuje.
6. Sprawdz, czy `system_status` nie wysyla nadmiarowych danych co sekunde.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_ws_client_manager"
/home/test/.platformio/penv/bin/pio test -e native -f "test_channel_subscriptions"
/home/test/.platformio/penv/bin/pio test -e native -f "test_system_snapshot_transport"
cd interface && npm run test:run
cd interface && DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e
```

Kryteria akceptacji:

- frontend system status dziala po odswiezeniu i po restarcie urzadzenia,
- WS nie przecieka klientow,
- payloady sa stabilne i opisane w kontrakcie,
- brak narastajacych queue drops w normalnym UI.

Commit:

```bash
git add src test interface packages docs
git commit -m "Harden system websocket snapshots"
```

## Podgol 11 - frontend shell, auth, statusbar i error handling

Cel:
Frontend ma byc uzyteczny, przewidywalny i odporny na utrate sesji/urzadzenia.

Kroki:

1. Przejrzyj:
   - `AppShell`,
   - `LoginScreen`,
   - `useLogin`,
   - `apiClient`,
   - `connectionState`,
   - `systemStatus`,
   - statusbar,
   - global error handling,
   - toasty/dialogi.
2. Testuj:
   - login admin/admin,
   - invalid login,
   - session expiry/401,
   - device offline,
   - slow endpoint,
   - restart progress,
   - mobile viewport,
   - desktop viewport.
3. Usprawnij UX:
   - jasne loading/error states,
   - brak pustych kart bez informacji,
   - akcje admin-only dobrze zablokowane,
   - nie ma nachodzacego tekstu,
   - widac stan polaczenia i restartu.
4. Nie buduj landing page. UI ma byc narzedziem operacyjnym.

Testy:

```bash
cd interface && npm run check
cd interface && npm run test:run
cd interface && npm run lint
cd interface && DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e
```

Kryteria akceptacji:

- UI przechodzi testy,
- E2E smoke loguje sie do realnego urzadzenia,
- widoczne bledy sa zrozumiale,
- mobile i desktop nie maja overlapow.

Commit:

```bash
git add interface docs
git commit -m "Improve frontend shell reliability"
```

## Podgol 12 - WiFi, mDNS, NTP, network recovery

Cel:
Siec jest stabilna po HTTPS, mDNS `plantcare.local` dziala, recovery nie psuje
runtime.

Kroki:

1. Przejrzyj framework WiFi:
   - `WiFiSettingsService`,
   - `WiFiStatus`,
   - `WiFiScanner`,
   - `APSettingsService`,
   - `NTPSettingsService`,
   - mDNS sync w `ESP32SvelteKit`.
2. Testuj:
   - `https://192.168.0.30`,
   - `https://plantcare.local`,
   - `/api/system/network`,
   - `/api/system/wifi/recover`,
   - `/rest/wifiStatus`,
   - `/rest/listNetworks`,
   - `/rest/scanNetworks`,
   - `/rest/ntpStatus`,
   - `/rest/ntpSettings`,
   - `/rest/time`.
3. Sprawdz, czy UI i SDK maja te same pola.
4. Upewnij sie, ze WiFi recovery loguje powod, wynik i nie odpala sie w petli.
5. Testuj AP/STA kontrakt bez niszczenia konfiguracji STA.

Testy:

```bash
curl -sk https://plantcare.local/api/system/network -H "Authorization: Bearer $TOKEN"
curl -sk https://192.168.0.30/rest/wifiStatus -H "Authorization: Bearer $TOKEN"
curl -sk -X POST https://192.168.0.30/api/system/wifi/recover -H "Authorization: Bearer $TOKEN"
/home/test/.platformio/penv/bin/pio test -e native -f "test_wifi_health"
cd interface && npm run test:run
```

Kryteria akceptacji:

- IP i mDNS dzialaja po HTTPS,
- network recovery jest bezpieczny,
- NTP/manual time kontrakt jest spojny z frontendem,
- brak regresji WiFi UI.

Commit:

```bash
git add src lib test interface packages docs
git commit -m "Verify network and mDNS reliability"
```

## Podgol 13 - sensors, compensation, binary data logger, charts

Cel:
SCD41/IMU/compensation/logger/charts daja poprawne dane i nie blokuja runtime.

Kroki:

1. Przejrzyj:
   - `src/sensors/**`,
   - `src/compensation/**`,
   - `src/system/datalogger/**`,
   - `/api/logs`,
   - frontend charts,
   - `binaryLogParser`.
2. Testuj:
   - snapshot sensorow przez WS,
   - log files list/download/delete,
   - charts parsing,
   - compensation GET/POST,
   - sensor read failure paths,
   - filesystem busy paths.
3. Dodaj sanity bounds:
   - CO2 zakres,
   - temp/humidity range,
   - stale data detection,
   - timestamp validity.
4. Upewnij sie, ze data logger nie trzyma FS mutex podczas dlugich operacji.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_sensor_snapshot_health"
/home/test/.platformio/penv/bin/pio test -e native -f "test_sensor_telemetry_packet"
/home/test/.platformio/penv/bin/pio test -e native -f "test_file_manager_path_utils"
cd interface && npm run test:run
curl -sk https://192.168.0.30/api/logs -H "Authorization: Bearer $TOKEN"
```

Kryteria akceptacji:

- UI charts nie padaja na pustych/niepelnych danych,
- endpointy logs sa odporne,
- compensation dziala runtime i po restarcie,
- brak FS lock contention w normalnym smoke.

Commit:

```bash
git add src test interface docs scripts
git commit -m "Validate sensor logging and charts"
```

## Podgol 14 - matrix LED, matrix manager, custom icons, menu

Cel:
Matrix LED i menu sa stabilne, szybkie i nie blokuja hot path.

Kroki:

1. Przejrzyj:
   - `src/matrix/**`,
   - `src/system/matrix/**`,
   - `src/system/matrix_manager/**`,
   - `src/api/matrix/**`,
   - frontend system/matrix.
2. Sprawdz wczesniejsza notatke:
   - MatrixTask ~32 FPS,
   - potencjalne podwojne locki w MatrixManagerService/MatrixLayerManager,
   - dirty flag/precomputed hash.
3. Testuj:
   - brightness,
   - rotation/auto-rotation,
   - live mode,
   - alarm overlay,
   - notification queue,
   - custom icon editor,
   - effects,
   - menu button actions.
4. Popraw UI Matrix jezeli jest MVP:
   - jasne preview,
   - walidacja custom icon,
   - save/apply feedback,
   - brak overlapow.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_matrix_state"
/home/test/.platformio/penv/bin/pio test -e native -f "test_matrix_runtime_applier"
/home/test/.platformio/penv/bin/pio test -e native -f "test_matrix_layer_manager"
/home/test/.platformio/penv/bin/pio test -e native -f "test_matrix_menu"
cd interface && npm run test:run
cd interface && DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e -- tests/e2e/matrix.spec.ts
```

Kryteria akceptacji:

- matrix hot path nie ma niepotrzebnego contention,
- ustawienia dzialaja runtime i po restarcie,
- UI matrix jest uzyteczne i testowane.

Commit:

```bash
git add src test interface docs
git commit -m "Harden matrix display runtime"
```

## Podgol 15 - alarms end-to-end

Cel:
Alarmy dzialaja poprawnie dla sensorow, BLE, WiFi sensing i Shelly, maja dobry
cooldown/status/notyfikacje i sa testowalne.

Kroki:

1. Przejrzyj:
   - `src/alarms/**`,
   - `src/api/alarms/**`,
   - alarm frontend,
   - alarm docs,
   - notification bridges,
   - Shelly bridge,
   - BLE data provider.
2. Testuj reguly:
   - CO2 above/below,
   - temperature above/below,
   - humidity above/below,
   - WiFi variance,
   - BLE temp/humidity/battery/RSSI,
   - Shelly status/power jesli model to wspiera,
   - invalid rule,
   - max rules,
   - cooldown,
   - severity,
   - includeStatus.
3. Dodaj testy na edge cases:
   - NaN/invalid sensor,
   - stale BLE,
   - brak Shelly,
   - duplicate names,
   - puste rules,
   - duzy payload.
4. Upewnij sie, ze alarm snapshot nie robi nadmiarowego `strcmp`/copy w hot path,
   jezeli wczesniejsza analiza to potwierdzi.
5. Frontend:
   - rule modal ma walidacje,
   - status current value jest jasny,
   - kanal notyfikacji i cooldown sa czytelne,
   - bledy save sa widoczne.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_alarm_logic"
/home/test/.platformio/penv/bin/pio test -e native -f "test_alarm_edge_cases"
/home/test/.platformio/penv/bin/pio test -e native -f "test_alarm_evaluator"
/home/test/.platformio/penv/bin/pio test -e native -f "test_alarm_rules_serializer"
/home/test/.platformio/penv/bin/pio test -e native -f "test_alarm_config_json"
cd interface && npm run test:run
curl -sk https://192.168.0.30/api/alarms/rules?includeStatus=1 -H "Authorization: Bearer $TOKEN"
```

Kryteria akceptacji:

- alarmy maja pelne testy edge cases,
- endpointy sa stabilne,
- UI rule management jest uzyteczne,
- alarm status jest spojny z WS i REST.

Commit:

```bash
git add src test interface docs scripts
git commit -m "Verify alarm engine end to end"
```

## Podgol 16 - notifications Telegram/Webhook/Pushover/Heartbeat

Cel:
Kanaly notyfikacji maja stabilne workers, retry, backoff, test send i status.

Kroki:

1. Przejrzyj:
   - `src/notifications/**`,
   - `src/system/health/heartbeat/**`,
   - `src/api/notifications/**`,
   - `src/api/heartbeat/**`,
   - Telegram commands,
   - frontend notification pages.
2. Testuj:
   - config GET/POST,
   - test send endpointy,
   - missing credentials,
   - invalid token/url,
   - TLS verify,
   - worker queue full,
   - retry/backoff,
   - shutdown/restart while worker active,
   - heartbeat disabled/enabled/test.
3. Nie wysylaj realnych spam notification jezeli credentiali brak albo nie ma
   bezpiecznego targetu. Uzyj mock/webhook listener tam, gdzie trzeba.
4. Telegram commands:
   - `/status`,
   - `/health`,
   - `/alarms`,
   - `/ble`,
   - `/matrix`,
   - `/reboot`,
   - `/users`,
   - invalid command,
   - authorization chat id.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_telegram_queue"
/home/test/.platformio/penv/bin/pio test -e native -f "test_telegram_backoff"
/home/test/.platformio/penv/bin/pio test -e native -f "test_telegram_send_validator"
/home/test/.platformio/penv/bin/pio test -e native -f "test_pushover_send_validator"
/home/test/.platformio/penv/bin/pio test -e native -f "test_notification_runtime_reconciler"
/home/test/.platformio/penv/bin/pio test -e native -f "test_notification_test_job_scheduler"
python scripts/diagnostics/check_heartbeat.py
cd interface && npm run test:run
```

Kryteria akceptacji:

- worker status jest widoczny w diagnostics/UI,
- invalid config nie crashuje,
- retry/backoff sa testowane,
- alarm notifier nie blokuje alarm engine.

Commit:

```bash
git add src test interface scripts docs
git commit -m "Harden notification workers"
```

## Podgol 17 - Shelly integration

Cel:
Shelly devices/control/worker sa stabilne, testowalne i dobrze zintegrowane z
alarmami.

Kroki:

1. Przejrzyj:
   - `src/shelly/**`,
   - `src/api/shelly/**`,
   - frontend Shelly,
   - SDK Shelly,
   - alarm Shelly bridge.
2. Testuj:
   - add device,
   - update device,
   - delete device,
   - invalid IP/host,
   - duplicate ID,
   - control relay,
   - timeout/offline device,
   - worker polling,
   - restart persistence,
   - alarm action/control.
3. Jezeli nie ma realnego Shelly w sieci, uzyj mocka testowego i jasno zapisz
   ograniczenie w raporcie.
4. Usprawnij UI:
   - status offline/online,
   - ostatni blad,
   - test control feedback,
   - walidacja IP/URL.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_shelly_config_json"
/home/test/.platformio/penv/bin/pio test -e native -f "test_shelly_command_bridge_contract"
/home/test/.platformio/penv/bin/pio test -e native -f "test_ip_validator"
python scripts/diagnostics/check_shelly.py
python scripts/diagnostics/trigger_shelly.py
cd interface && npm run test:run
```

Kryteria akceptacji:

- Shelly kontrakt firmware/frontend/SDK spojny,
- offline/timeout nie blokuje runtime,
- alarm bridge ma testy.

Commit:

```bash
git add src test interface packages scripts docs
git commit -m "Verify Shelly integration"
```

## Podgol 18 - BLE scanner i BLE config

Cel:
BLE dziala po wlaczeniu, discovery i whitelist/cache sa stabilne, a radio
coexistence z WiFi/AP jest bezpieczne.

Kroki:

1. Przejrzyj:
   - `src/ble/**`,
   - `src/api/ble/**`,
   - `BleSettingsService`,
   - `BleScannerCache`,
   - parsers `BTHome`, `ThermoPro`,
   - frontend Bluetooth,
   - SDK BLE.
2. Wlacz BLE przez `/api/ble/settings`, z restartem jesli wymagany.
3. Testuj:
   - GET settings/status,
   - enable/disable,
   - discovery start/stop,
   - timeout min/max,
   - saved sensors,
   - discovery devices only for admin,
   - parser invalid payload,
   - stale lastSeen behavior przed/po NTP,
   - AP coexistence.
4. Sprawdz wczesniejsze ryzyko:
   - BLE runtime readings flush do globalnego RTC/config/CRC co 500 ms.
   - Oddziel runtime readings od persistent config, jezeli nadal wystepuje.
5. Dodaj runtime metrics:
   - adv count,
   - valid packets,
   - parser errors,
   - cache drops,
   - mutex timeouts,
   - scanner running.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_ble_config_logic"
/home/test/.platformio/penv/bin/pio test -e native -f "test_ble_whitelist"
/home/test/.platformio/penv/bin/pio test -e native -f "test_ble_device_cache"
/home/test/.platformio/penv/bin/pio test -e native -f "test_bthome_parser"
/home/test/.platformio/penv/bin/pio test -e native -f "test_tp_parser"
python scripts/diagnostics/check_ble_config.py
python scripts/diagnostics/trigger_ble.py
```

Kryteria akceptacji:

- BLE enable/disable przechodzi z restartem,
- discovery dziala i konczy sie,
- cache nie generuje lock contention,
- BLE runtime nie mieli stale globalnego configu bez potrzeby.

Commit:

```bash
git add src test interface packages scripts docs
git commit -m "Stabilize BLE scanner runtime"
```

## Podgol 19 - WiFi sensing RSSI i CSI streaming

Cel:
WiFi sensing i CSI sa mierzalne, stabilne, nie zalewaja WS i maja spojny format
frontend/backend.

Kroki:

1. Przejrzyj:
   - `src/wifisensing/**`,
   - `src/api/wifisensing/**`,
   - `docs/engineering/integrations/csi.md`,
   - `tools/csi_client.py`,
   - frontend `interface/src/lib/features/wifisensing/**`.
2. Testuj RSSI sensing:
   - enable/disable,
   - sample interval,
   - variance threshold,
   - stats,
   - alarm source.
3. Testuj CSI:
   - `/ws/csi` auth,
   - frontend consumer active/inactive,
   - queue lazy enable/disable,
   - binary wire format,
   - parser frontendowy,
   - batching do transportu.
4. Zweryfikuj wczesniejsza notatke:
   - CSI batchuje pakiety w workerze, ale transport moze rozbijac batch.
   - Jezeli nadal prawda, zachowaj batching az do wire format albo swiadomie
     ogranicz rate.
5. Dodaj metrics:
   - packets/sec,
   - batches/sec,
   - dropped batches,
   - queue depth/drops,
   - consumer count,
   - calibration/state.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_wifisensing"
python scripts/diagnostics/trigger_wifisensing.py
python tools/csi_client.py --url https://192.168.0.30 --username admin --password admin --duration 60
cd interface && npm run test:run
```

Kryteria akceptacji:

- CSI stream dziala przez HTTPS/WSS,
- frontend parser nie gubi ramek,
- queue drops nie rosna w normalnym podgladzie,
- RSSI sensing jest stabilne i opisane.

Commit:

```bash
git add src test interface tools scripts docs
git commit -m "Harden WiFi sensing and CSI streaming"
```

## Podgol 20 - opcjonalnie CSI jako zrodlo alarmow

Cel:
Zdefiniowac i ewentualnie wdrozyc CSI motion/person detection jako zrodlo alarmow
bez robienia naiwnych false positives.

Ten podgol jest duzy. Jezeli wdrozenie zagrozi stabilizacji release, zrob
komercyjna specyfikacje i test harness, commituj ja, a implementacje zostaw jako
osobny przyszly etap. Nie blokuj calego commercial-grade hardeningu, jezeli
brakuje danych treningowych albo czasu na falsyfikacje.

Minimalna definicja, jesli implementujesz:

- nie alarmuj na raw CSI amplitude,
- zbuduj score z okna czasowego,
- baseline adaptacyjny po warmup/calibration,
- noise floor,
- hysteresis,
- debounce,
- min duration,
- cooldown,
- confidence,
- ignorowanie okresow WiFi reconnect/CSI reset,
- osobne stany: idle, calibrating, monitoring, motion_candidate, motion_confirmed,
  noisy_environment, unavailable.

Testy:

- offline analysis na zapisanych CSI frame,
- symulowane noisy data,
- symulowany ruch,
- false positive przy pustym pokoju,
- frontend status i alarm rule source,
- runtime metrics.

Kryteria akceptacji dla implementacji:

- alarm source `csi_motion_score` albo podobny jest w kontrakcie,
- ma testy natywne algorytmu,
- ma frontend rule UI,
- ma diagnostics,
- nie podnosi alarmow przy oczywistym szumie.

Kryteria akceptacji dla deferral:

- istnieje dokument `docs/engineering/integrations/csi-alarms-plan.md`,
- opisuje sygnal, progi, dane potrzebne do kalibracji, test harness i ryzyka,
- obecny kod nie jest naruszony.

Commit:

```bash
git add src test interface docs tools scripts
git commit -m "Define CSI alarm detection path"
```

## Podgol 21 - AirMouse, keyboard, jiggler, USB terminal

Cel:
USB HID i USB terminal dzialaja po wlaczeniu, po restarcie i bez zawieszania
runtime.

Kroki:

1. Przejrzyj:
   - `src/airmouse/**`,
   - `src/keyboard/**`,
   - `src/usb_terminal/**`,
   - `src/api/airmouse/**`,
   - `src/api/keyboard/**`,
   - `src/api/usb_terminal/**`,
   - frontend USB features.
2. Wlacz kolejno:
   - keyboard,
   - AirMouse,
   - jiggler,
   - USB terminal.
3. Jezeli zmiana wymaga restartu, wykonaj restart i potwierdz powrot.
4. Testuj:
   - keyboard type,
   - key press,
   - airmouse status,
   - calibrate,
   - jiggler settings,
   - USB terminal config,
   - `/ws/usbterminal`,
   - oversize frame,
   - concurrent clients,
   - disconnect cleanup,
   - busy command,
   - telegram transport jesli skonfigurowany.
5. Upewnij sie, ze logi USB nie znikaja przez DTR/RTS w zwyklym monitorze albo
   dokumentuj wymagany tryb.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_keyboard"
/home/test/.platformio/penv/bin/pio test -e native -f "test_usb_terminal_input"
/home/test/.platformio/penv/bin/pio test -e native -f "test_macro_logic"
cd interface && npm run test:run
```

Kryteria akceptacji:

- funkcje USB wlaczaja sie i wylaczaja przewidywalnie,
- restart po zmianie configu nie psuje urzadzenia,
- UI USB features jest uzyteczne,
- endpointy sa zabezpieczone admin-only tam, gdzie wykonuje sie akcje HID.

Commit:

```bash
git add src test interface docs scripts
git commit -m "Validate USB feature runtime"
```

## Podgol 22 - macro engine

Cel:
Makra sa bezpieczne, testowalne, dobrze walidowane i nie moga zablokowac runtime.

Kroki:

1. Przejrzyj:
   - `src/macros/**`,
   - `src/api/macros/**`,
   - frontend macros,
   - macro docs.
2. Testuj:
   - list scripts,
   - create/update,
   - get content,
   - run,
   - stop,
   - status,
   - delete,
   - invalid syntax,
   - long script,
   - command timeout,
   - boot script setting,
   - restart persistence,
   - concurrent run/stop.
3. Dodaj limity:
   - max script size,
   - max command count,
   - max runtime,
   - output/log throttling,
   - safe errors.
4. UI:
   - editor waliduje,
   - help commands jest aktualny,
   - run/stop status jest jasny.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_macro_config"
/home/test/.platformio/penv/bin/pio test -e native -f "test_macro_repository"
/home/test/.platformio/penv/bin/pio test -e native -f "test_macro_logic"
cd interface && npm run test:run
curl -sk https://192.168.0.30/api/macros/status -H "Authorization: Bearer $TOKEN"
```

Kryteria akceptacji:

- makro nie moze zawiesic taska,
- run/stop dziala,
- payload limits sa testowane,
- UI jest zgodne z kontraktem.

Commit:

```bash
git add src test interface docs scripts
git commit -m "Harden macro engine workflows"
```

## Podgol 23 - UDP push i heartbeat monitoring

Cel:
UDP push i heartbeat sa przewidywalne, maja test listener i nie blokuja sieci.

Kroki:

1. Przejrzyj:
   - `src/udp/**`,
   - `src/api/udp/**`,
   - `src/system/health/heartbeat/**`,
   - frontend UDP/heartbeat.
2. Testuj:
   - GET/POST config,
   - test push,
   - invalid host/port,
   - listener receive,
   - worker restart,
   - disable while worker active,
   - pushNow while disabled,
   - heartbeat test,
   - webhook errors.
3. Upewnij sie, ze `UdpPusher` locki i cleanup semaphores sa bezpieczne.

Testy:

```bash
python scripts/diagnostics/udp_listener.py --timeout 60
python scripts/diagnostics/check_udp.py
curl -sk -X POST https://192.168.0.30/api/udp/test -H "Authorization: Bearer $TOKEN"
/home/test/.platformio/penv/bin/pio test -e native
cd interface && npm run test:run
```

Kryteria akceptacji:

- UDP test jest automatyczny,
- worker start/stop nie zostawia taskow,
- heartbeat ma jasny status i retry.

Commit:

```bash
git add src test interface scripts docs
git commit -m "Verify UDP and heartbeat workflows"
```

## Podgol 24 - power management, sleep, wake, thermal

Cel:
Power management i thermal protection sa bezpieczne, testowalne i nie gasza
urzadzenia w trakcie aktywnych testow bez jasnego powodu.

Kroki:

1. Przejrzyj:
   - `src/system/power/**`,
   - `src/system/thermal/**`,
   - `src/api/power/**`,
   - frontend power.
2. Testuj:
   - `/rest/power/status`,
   - `/rest/power/config`,
   - `/rest/power/hygieneSleep`,
   - `/rest/sleep`,
   - activity tracking,
   - grace after boot,
   - wake reason,
   - thermal monitor thresholds.
3. Upewnij sie, ze endpoint activity nie resetuje power timer w sposob falszywy
   tam, gdzie nie powinien.
4. UI:
   - jasny status sleep enabled/disabled,
   - confirmation przed sleep/restart/factory reset,
   - progress po restarcie.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_power_activity_logging"
/home/test/.platformio/penv/bin/pio test -e native -f "test_thermal_monitor"
curl -sk https://192.168.0.30/rest/power/status -H "Authorization: Bearer $TOKEN"
cd interface && npm run test:run
```

Kryteria akceptacji:

- power config dziala po restarcie,
- sleep nie odpala sie podczas aktywnego UI/WS,
- thermal status jest widoczny diagnostycznie.

Commit:

```bash
git add src test interface docs
git commit -m "Validate power and thermal management"
```

## Podgol 25 - file manager i filesystem safety

Cel:
File manager jest bezpieczny, odporny na path traversal, upload errors i locki FS.

Kroki:

1. Przejrzyj:
   - `src/filemanager/**`,
   - `src/api/filemanager/**`,
   - frontend file manager,
   - file manager state/store.
2. Testuj:
   - list root,
   - list `/data`,
   - download small file,
   - upload small file do bezpiecznej sciezki testowej,
   - remove test file,
   - path traversal,
   - protected log file,
   - insufficient space,
   - interrupted upload,
   - concurrent download/list.
3. Nie usuwaj realnych logow uzytkownika bez kopii.
4. Upewnij sie, ze FS mutex nie jest trzymany podczas calego duzego transferu,
   chyba ze kod jest do tego swiadomie zaprojektowany i ma timeout.

Testy:

```bash
/home/test/.platformio/penv/bin/pio test -e native -f "test_file_manager_path_utils"
cd interface && npm run test:run
curl -sk "https://192.168.0.30/rest/fs/list?dir=/" -H "Authorization: Bearer $TOKEN"
```

Kryteria akceptacji:

- path traversal odrzucony,
- upload/download/remove dzialaja,
- UI ma feedback i nie myli `dir`/`path`,
- brak FS lock contention w smoke.

Commit:

```bash
git add src test interface docs scripts
git commit -m "Harden filesystem manager"
```

## Podgol 26 - frontend feature pages uzytecznosc i polish MVP cleanup

Cel:
Najwazniejsze strony nie wygladaja i nie dzialaja jak MVP. Maja byc operacyjne,
czytelne i odporne na bledy.

Strony:

- dashboard,
- system status,
- logs,
- matrix,
- alarms,
- bluetooth,
- WiFi,
- WiFi sensing,
- Shelly,
- notifications,
- UDP,
- heartbeat,
- charts,
- file manager,
- compensation,
- power,
- USB features,
- macros,
- user/security.

Kroki:

1. Przejrzyj kazda strone w desktop i mobile.
2. Popraw:
   - loading states,
   - empty states,
   - error states,
   - stale/offline state,
   - disabled controls,
   - validation,
   - save/apply/restart feedback,
   - layout density,
   - overlap tekstu,
   - zbyt ozdobne/nieoperacyjne karty.
3. Nie tworz marketing landing page. Pierwszy ekran ma byc realnym narzedziem.
4. Dodaj testy dla hooks/modeli, nie tylko snapshot UI.
5. Playwright: dodaj lub rozszerz smoke po najwazniejszych stronach.

Testy:

```bash
cd interface && npm run lint
cd interface && npm run check
cd interface && npm run test:run
cd interface && DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e
```

Kryteria akceptacji:

- E2E przechodzi,
- brak oczywistych UI overlapow,
- strony maja uzyteczne stany bledow,
- kontrakt z backendiem jest jasny i testowany.

Commit:

```bash
git add interface docs
git commit -m "Polish frontend feature workflows"
```

## Podgol 27 - TypeScript SDK jako kontrakt publiczny

Cel:
`packages/device-sdk` nie moze byc przypadkowym fragmentem. Ma byc spojny z
firmware i frontendem.

Kroki:

1. Przejrzyj SDK:
   - auth,
   - client/errors,
   - system,
   - network,
   - shelly,
   - ble,
   - matrix,
   - ws.
2. Dodaj brakujace klienty dla najwazniejszych endpointow:
   - diagnostics,
   - power,
   - alarms,
   - notifications,
   - macros,
   - WiFi sensing,
   - logs, jesli sensowne.
3. Dodaj typy zgodne z API contract.
4. Dodaj testy SDK dla:
   - auth,
   - retries/errors,
   - URL building HTTPS/WSS,
   - response parsing.

Testy:

```bash
cd packages/device-sdk && npm test
cd interface && npm run test:run
/home/test/.platformio/penv/bin/pio test -e native
```

Jezeli SDK nie ma jeszcze test scriptow, dodaj minimalna konfiguracje albo opisz
dlaczego testy sa wykonywane przez interface tests.

Kryteria akceptacji:

- SDK pathy sa zgodne z manifestem,
- WSS builder obsluguje HTTPS,
- najwazniejsze typy nie dryfuja od API.

Commit:

```bash
git add packages interface docs src test
git commit -m "Align device SDK with API contract"
```

## Podgol 28 - automatyczny device smoke suite

Cel:
Musi istniec jeden smoke test, ktory agent moze uruchomic przeciwko realnemu
urzadzeniu i ktory daje pass/fail dla glownego kontraktu.

Stworz lub popraw:

- `scripts/tests/device_smoke.py`

Wymagania smoke:

- HTTPS only,
- JWT login,
- retries,
- 401 reauth,
- JSON schema light validation,
- latency measurement,
- memory snapshot before/after,
- optional WSS checks,
- test read-only endpointow,
- test safe POST endpointow z backup/restore configu,
- test restart optional flag,
- raport JSON/Markdown.

Read-only coverage:

- `/api/system/info`,
- `/api/system/tasks?details=1`,
- `/api/system/network`,
- `/api/config`,
- `/rest/power/status`,
- `/api/matrix/settings`,
- `/api/alarms/rules?includeStatus=1`,
- `/api/ble/status`,
- `/api/wifisensing/config`,
- `/api/shelly/devices`,
- `/api/heartbeat`,
- `/api/udp`,
- `/api/notifications/settings`,
- `/api/airmouse/status`,
- `/api/keyboard/config`,
- `/api/macros/status`,
- `/api/compensation`,
- `/api/usbterminal/config`,
- `/api/logs`,
- `/rest/logs/tail?lines=50`.

Safe write coverage z backup/restore:

- `/api/config` log level no-op,
- `/api/matrix/settings` no-op save,
- `/api/ble/settings` no-op save,
- `/api/wifisensing/config` no-op save,
- `/api/alarms/rules` backup/restore,
- `/api/udp` disabled test config, jezeli bezpieczne,
- `/api/heartbeat` disabled test config, jezeli bezpieczne.

Testy:

```bash
python scripts/tests/device_smoke.py --device-url https://192.168.0.30 --username admin --password admin --read-only
python scripts/tests/device_smoke.py --device-url https://192.168.0.30 --username admin --password admin --safe-writes
```

Kryteria akceptacji:

- smoke suite przechodzi,
- raport pokazuje latency, failures, heap before/after,
- jest bezpieczny dla konfiguracji uzytkownika.

Commit:

```bash
git add scripts docs src test interface packages
git commit -m "Add comprehensive device smoke suite"
```

## Podgol 29 - stress, soak, fault injection

Cel:
System przechodzi dluzsze testy i awaryjne scenariusze.

Scenariusze:

- 1h soak z funkcjami wlaczonymi,
- 30 min endpoint stress,
- 10 restartow,
- WSS reconnect loop,
- wiele klientow frontendowych,
- invalid payload flood z rate limit,
- upload/download loop malych plikow,
- BLE scan start/stop loop,
- CSI connect/disconnect loop,
- macro run/stop loop,
- notification test invalid credentials,
- WiFi recovery call while online.

Kroki:

1. Rozszerz `scripts/tests/stress_test.py` i `soak_test.py`, zeby uzywaly
   aktualnych endpointow.
2. Dodaj raport koncowy:
   - success/fail,
   - request count,
   - avg/p95 latency,
   - heap drift,
   - min stack watermark,
   - lock contention counters,
   - WS drops,
   - reset detection.
3. Wszystkie testy musza byc nieinteraktywne.

Testy:

```bash
python scripts/tests/stress_test.py --cycles 100 --delay 0.01
python scripts/tests/soak_test.py --duration 1h --interval 10s --plot
python scripts/tests/device_smoke.py --safe-writes
```

Kryteria akceptacji:

- brak resetow,
- brak WDT,
- heap drift w granicach ustalonych w Podgolu 6,
- p95 latency akceptowalne i zapisane,
- failures = 0 albo kazdy failure ma ticket/commit naprawczy.

Commit:

```bash
git add scripts docs src test interface
git commit -m "Add stress and soak validation"
```

## Podgol 30 - build, size, partitions, dependency hygiene

Cel:
Build jest powtarzalny, rozmiar miesci sie w partycjach, zaleznosci sa czyste.

Kroki:

1. Firmware:
   - `pio run -e waveshare_esp32s3_matrix`,
   - `SKIP_UI=1 pio run -e waveshare_esp32s3_matrix`,
   - sprawdz size,
   - sprawdz partitions,
   - sprawdz warnings.
2. Frontend:
   - `npm run build`,
   - `npm run size`,
   - `npm run unused:check`,
   - `npm run deps:check`.
3. Sprawdz, czy `build_interface.py` poprawnie embeduje UI.
4. Nie commituj `node_modules`, build outputow i logow, chyba ze repo juz
   swiadomie wersjonuje konkretne artefakty.

Testy:

```bash
/home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix
/bin/bash -lc "SKIP_UI=1 /home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix"
cd interface && npm run quality:frontend
```

Kryteria akceptacji:

- firmware build przechodzi,
- frontend build przechodzi,
- brak nowych warnings krytycznych,
- app miesci sie w partycjach.

Commit:

```bash
git add platformio.ini sdkconfig.defaults partitions scripts interface package* docs src test
git commit -m "Verify production build hygiene"
```

## Podgol 31 - dokumentacja operacyjna i release checklist

Cel:
Po hardeningu zostaje dokument, ktory pozwala powtorzyc testy i utrzymac jakosc.

Dodaj/zaktualizuj:

- `docs/engineering/operations/testing.md`,
- `docs/engineering/operations/security_hardening.md`,
- `docs/engineering/operations/runtime-diagnostics.md`,
- `docs/engineering/api-contract.md`,
- user guide tylko tam, gdzie UI/funkcja realnie sie zmienila.

Dokumentacja musi zawierac:

- jak zbudowac,
- jak flashowac,
- jak logowac sie po HTTPS/JWT,
- jak uruchomic smoke,
- jak uruchomic stress/soak,
- jak interpretowac heap/stack/lock metrics,
- jak sprawdzic coredump,
- jak wlaczyc BLE/CSI/USB/makra bez zgubienia configu,
- jakie sa kryteria release pass/fail.

Test:

- wykonaj checklist z dokumentu na realnym urzadzeniu.

Kryteria akceptacji:

- nowy agent/czlowiek moze powtorzyc caly proces,
- wszystkie komendy sa aktualne dla `https://192.168.0.30`,
- brak starych HTTP defaultow.

Commit:

```bash
git add docs scripts README.md interface src test
git commit -m "Document release diagnostics workflow"
```

## Podgol 32 - finalny full regression gate

Cel:
Ostateczna bramka: wszystko przechodzi razem.

Wykonaj w tej kolejnosci:

```bash
git status --short
/home/test/.platformio/penv/bin/pio test -e native
/home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix
/bin/bash -lc "SKIP_UI=1 /home/test/.platformio/penv/bin/pio run -e waveshare_esp32s3_matrix"
cd interface && npm run lint
cd interface && npm run check
cd interface && npm run test:run
cd interface && npm run build
cd interface && npm run size
cd interface && DEVICE_URL=https://192.168.0.30 TEST_USERNAME=admin TEST_PASSWORD=admin npm run test:e2e
python scripts/tests/device_smoke.py --device-url https://192.168.0.30 --username admin --password admin --read-only
python scripts/tests/device_smoke.py --device-url https://192.168.0.30 --username admin --password admin --safe-writes
python scripts/tests/stress_test.py --cycles 100 --delay 0.01
python scripts/tests/soak_test.py --duration 1h --interval 10s
```

Po testach:

1. Pobierz final diagnostics:
   - system info,
   - tasks details,
   - heap,
   - mutexes,
   - endpoint metrics,
   - logs tail.
2. Porownaj z baseline z Podgolu 0.
3. Zrob finalny raport `docs/engineering/operations/final-validation-report.md`
   albo `artifacts/diagnostics/<timestamp>/final-report.md`.
4. Jezeli raport jest w `artifacts`, nie commituj duzych raw logow. Commituj tylko
   krotki raport release, jesli pasuje do repo.

Kryteria akceptacji:

- wszystkie testy przechodza,
- urzadzenie po soak nadal online,
- brak WDT/reset/panic,
- heap/stack stabilne,
- lock/WS drops w normie,
- frontend E2E dziala na realnym HTTPS device,
- kontrakt API zsynchronizowany,
- dokumentacja aktualna.

Commit:

```bash
git add docs scripts src test interface packages platformio.ini sdkconfig.defaults
git commit -m "Complete commercial grade validation"
git status --short
```

Nie rob push.

## Definicja koncowego sukcesu

Goal jest skonczony dopiero gdy:

- repo ma przechodzacy firmware build,
- `pio test -e native` przechodzi,
- frontend lint/check/test/build przechodzi,
- Playwright E2E przechodzi przeciwko `https://192.168.0.30`,
- device smoke read-only i safe-writes przechodzi,
- stress i soak przechodza bez resetu i WDT,
- runtime metrics sa dostepne i mierzalne,
- memory/stack/lock/WS metrics sa stabilne,
- wszystkie glowne funkcje byly wlaczone i sprawdzone,
- endpointy sa zsynchronizowane z frontendem i SDK,
- logi sa czytelne i bez sekretow,
- dokumentacja testowania jest aktualna,
- po kazdym podgolu jest commit,
- nie wykonano push.

Jezeli cos nie przechodzi, nie koncz goal jako sukces. Napraw, dodaj test i commit.
