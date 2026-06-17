# UX Refactor Plan - MatrixHub Frontend

## Goal

Doprowadzić frontend MatrixHub do commercial-grade poziomu: spójny zapis ustawień, czysty minimalny design, dane i ustawienia na pierwszym miejscu, stabilny runtime na ESP32/LittleFS, mierzalne ładowanie strony oraz quality gates, które łapią regresje przed produkcją.

Menu boczne zostaje w obecnej strukturze. Plan nie zakłada dodawania nowych tabów ze skrótami ani marketingowych ekranów. Refaktor ma porządkować istniejące przepływy i komponenty.

## Operacyjna zasada checkpointów

Wymaganie "push bez commit" jest technicznie sprzeczne z działaniem Gita: `git push` nie wysyła niezakomitowanych zmian. Dlatego po każdym etapie stosujemy checkpoint no-commit:

1. Uruchomić komendy weryfikujące z etapu.
2. Sprawdzić `git status --short` i `git diff --check`.
3. Zapisać awaryjny patch: `git diff --binary > docs/refactor-checkpoints/phase-XX.patch`.
4. Nie robić commita automatycznie. Jeśli później zostanie zatwierdzony commit, wtedy dopiero `git push` realnie zabezpieczy kod na remote.

## Obecny stan audytu

### Główne blokery produkcyjne

- `npm run lint` nie przechodzi. Prettier zgłasza 5 plików: `project.inlang/.meta.json`, `project.inlang/README.md`, `src/lib/features/system/status/components/NetworkDiagnosticsModal.svelte`, `src/lib/features/wifi/sta/useWifiManagement.svelte.ts`, `src/routes/+page.svelte`.
- `npm run unused:check` nie przechodzi. Knip zgłasza nieużywaną zależność `jwt-decode`, 14 nieużywanych eksportów, 3 nieużywane typy oraz nieaktualny wpis ignore dla `@rollup/rollup-darwin-arm64`.
- Build produkcyjny przechodzi, ale Vite ostrzega o pojedynczym dużym chunku: klient ma ok. 1.14 MB minified / 324 KB gzip. Obecny budżet przechodzi, ale margines dla ESP32/LittleFS jest ograniczony.
- Dependency-cruiser przechodzi tylko względem baseline'u. Baseline zawiera znane cykle importów, więc architektura ma dług, który nie powinien zostać utrwalony.
- Matrix LED nie ma testu UX kontraktu zapisu: testy jednostkowe sprawdzają model i endpoint, ale nie łapią mieszania manual-save, auto-save i modal-save.

### Matrix LED - problemy krytyczne

- `MatrixEffects.svelte` ma cichy autosave wybranego efektu. Jeśli zapis się nie powiedzie i `store.hasChanges` zostaje true, `pendingEffectAutosave` może uruchamiać kolejne zapisy bez końca. Ryzyko: spam endpointu, spam toastów błędu i dodatkowe obciążenie urządzenia. Źródło: `MatrixEffects.svelte` linie 165-170 oraz 217-227.
- Ten sam ekran miesza trzy semantyki zapisu:
  - wybór efektu/kategorii zapisuje się cicho i natychmiast,
  - kolory, speed, toggle efektów i część ustawień czekają na ręczny Save,
  - edytor ikon zapisuje przez modal natychmiast do endpointu.
- Lewa karta `Matrix LED Settings` mutuje ten sam store, ale nie ma własnego przycisku Save. Jedyny główny Save jest w karcie `Visual Effects`, więc użytkownik może zmienić jasność/rotację/menu i nie zobaczyć akcji zapisu w kontekście tej karty.
- `menu_enabled` jest pokazane jako zablokowany toggle, a frontend przy każdym zapisie wymusza `menu_enabled: true`. To ukrywa decyzję produktową i nadpisuje stan backendu niezależnie od payloadu. Źródło: `useMatrixSettings.svelte.ts` linie 31-35 oraz `MatrixSettings.svelte` linie 147-153.
- Sekcje mają dużo opisów powtarzających nazwę pola. To stoi w konflikcie z zasadą minimalnego UI: ustawienia i dane pierwsze, opis tylko tam, gdzie usuwa niepewność.

### Svelte i ładowanie strony

- Projekt używa Svelte 5 i SvelteKit 2, co jest dobrym punktem startowym.
- `svelte.config.js` ma `bundleStrategy: 'single'`, prawdopodobnie pod LittleFS i firmware embed. Oficjalny SvelteKit rekomenduje domyślnie split-bundling dla większości aplikacji, ale tutaj single jest świadomym trade-offem. Nie zmieniać bez pomiaru na urządzeniu.
- `src/app.html` ma `data-sveltekit-preload-data="hover"`. Dla panelu urządzenia warto przetestować `tap`, bo hover może robić fałszywe preloady i potencjalnie odświeżać dane, których użytkownik nie otworzy.
- Svelte docs ostrzegają, że aktualizowanie stanu wewnątrz `$effect` zwykle komplikuje kod i może prowadzić do cykli. Matrix Effects robi kilka synchronizacji lokalnego stanu w `$effect`; część jest uzasadniona, ale autosave/retry powinien zostać usunięty albo przepisany na jawny event flow.

### UI/design system

- `BaseCard` ma slot actions, ale nie istnieje wspólny kontrakt `SettingsCard`: dirty state, Save, Reset/Discard, saving/error i aria-live status.
- `ContentBox` wygląda jak mini-karta w karcie. Przy wielu ekranach tworzy wizualną ciężkość i powiela powierzchnie zamiast prowadzić wzrok po danych.
- `FormToggle` importuje `ContentBox`, więc prymityw formularza decyduje o layoucie. To utrudnia minimalistyczny design i powoduje nested-card pattern.
- Globalne cienie kart, hover transform i tło z radial-gradient nadają aplikacji demo-dashboardowy charakter. Commercial-grade panel powinien być spokojniejszy, bardziej przewidywalny i mniej ruchomy.
- Dokumentacyjne screenshoty Matrix LED wyglądają na starszy stan UI/brandingu. To jest ryzyko dokumentacyjne przy wydaniu komercyjnym.

## Zasady docelowe frontendu

- Każda edytowalna karta ma widoczny i spójny zapis w swoim kontekście albo ekran ma jeden jawny sticky/page-level Save obejmujący wszystkie karty. Nie mieszamy obu modeli bez etykiet.
- Auto-save tylko dla intencji nazwanych jako natychmiastowe działanie, np. `Apply now`, nie dla zwykłego selecta.
- Modal nie zapisuje trwale "bokiem", jeśli reszta ekranu działa jako draft. Modal może zwrócić draft, a karta/strona zapisuje całość.
- Dane i kontrolki są pierwsze. Opisy krótkie, tylko dla ryzykownych albo nieoczywistych ustawień.
- Menu zostaje bez zmian strukturalnych.
- Layout nie używa kart w kartach jako domyślnego sposobu grupowania.
- Każda zmiana ustawień ma test: unit dla modelu, component/interaction dla dirty state, e2e dla realnej ścieżki użytkownika.

## Etapy refaktoryzacji

### Etap 0 - Ustabilizowanie quality gate

Cel: mieć zielony baseline przed dotykaniem UX.

Zakres:

- Naprawić Prettier w 5 wskazanych plikach.
- Posprzątać lub świadomie oznaczyć wyniki `knip`: `jwt-decode`, eksporty dashboard/navigation/system transport oraz typy.
- Zachować zielone `npm run check`, `npm run test:run -- src/lib/features/system/matrix src/lib/services/api/core/MatrixApiService.test.ts`, `npm run build`, `node ./scripts/check-build-size.mjs`, `npm run deps:check`.
- Nie usuwać baseline'u dependency-cruiser jeszcze, tylko opisać go jako dług architektoniczny.

Akceptacja:

- `npm run lint` przechodzi.
- `npm run unused:check` przechodzi albo ma celowy, mały allowlist z komentarzem.
- Build size nie przekracza obecnych budżetów: JS gzip 400 KB, CSS gzip 50 KB, total 1.5 MB.

Checkpoint:

- Zapisać patch `docs/refactor-checkpoints/phase-00-quality-gate.patch`.

### Etap 1 - Kontrakt zapisu ustawień

Cel: jeden spójny model zapisu dla wszystkich kart ustawień.

Zakres:

- Zdefiniować komponent/kontrakt `SettingsCard` albo rozszerzyć `BaseCard` o standardowe actions/footer:
  - Save z ikoną,
  - Reset/Discard, jeśli karta ma draft,
  - disabled gdy brak zmian,
  - loading podczas zapisu,
  - status/error w `aria-live`.
- Ustalić regułę:
  - karta edytowalna ma własny Save, albo
  - ekran ma jeden sticky Save obejmujący wszystkie karty.
- Dla Matrix LED rekomendacja: dwie karty, dwa widoczne Save przy nagłówku/stopce, oba zapisują wspólny endpoint, ale dirty state pokazuje się w kontekście karty, której pola zmieniono.
- Dodać testy komponentowe na:
  - Save widoczny w każdej edytowalnej karcie,
  - Save disabled bez zmian,
  - Reset przywraca ostatni saved snapshot,
  - podwójny klik Save nie robi dwóch requestów.

Akceptacja:

- Matrix LED nie ma już sytuacji, w której lewa karta zmienia ustawienia bez lokalnie widocznej akcji zapisu.
- `useSettings` pozostaje jednym źródłem prawdy dla dirty/saving/error, bez lokalnych obejść.

Checkpoint:

- `npm run check`
- testy komponentów ustawień
- patch `docs/refactor-checkpoints/phase-01-save-contract.patch`

### Etap 2 - Matrix LED: naprawa flow i logiki

Cel: Matrix LED ma przewidywalny, testowalny i bezpieczny zapis.

Zakres:

- Usunąć cichy autosave z wyboru efektu albo zamienić go na jawny przycisk `Apply effect` z debounce/rate limit i bez retry loop.
- Naprawić pętlę `pendingEffectAutosave`: po błędzie zapis nie może samoczynnie ponawiać requestu bez nowej intencji użytkownika.
- Ujednolicić modal ikon:
  - albo zapisuje tylko draft ikon i wymaga Save w karcie,
  - albo jest wyraźnie oznaczony jako natychmiastowy zapis i nie zostawia niejasnego dirty state.
- Przenieść definicje kategorii efektów i presetów kolorów z komponentu do modelu/testowalnej konfiguracji.
- Rozstrzygnąć `menu_enabled`:
  - jeśli menu fizyczne ma być zawsze włączone, usunąć udawany disabled toggle i pokazać neutralny status,
  - jeśli ma być konfigurowalne, pozwolić na zapis wartości i dodać backend/frontend test.
- Dodać frontendową walidację payloadu Matrix:
  - brightness 2..255,
  - rotation 0..3,
  - effect mode 0..69,
  - speed 50 ms..24 h,
  - custom icons: 3 sloty, 64 piksele albo pusty slot.
- Użyć istniejącej możliwości `apiClient` z `zod` schema dla odpowiedzi `/api/matrix/settings`.

Akceptacja:

- Test symulujący błąd POST dla zmiany efektu potwierdza dokładnie jeden request i brak retry loop.
- Test e2e Matrix: zmiana jasności, rotacji, koloru menu, efektu i ikon ma jasny Save/Cancel i po reloadzie stan jest zgodny.
- Nie ma ukrytego nadpisywania `menu_enabled` bez decyzji produktowej zapisanej w teście.

Checkpoint:

- `npm run test:run -- src/lib/features/system/matrix src/lib/services/api/core/MatrixApiService.test.ts`
- właściwy test e2e Matrix
- patch `docs/refactor-checkpoints/phase-02-matrix-led.patch`

### Etap 3 - Minimalny commercial-grade UI system

Cel: uprościć wygląd bez zmiany menu i bez utraty funkcji.

Zakres:

- Ograniczyć globalne cienie i hover transform kart. Karty mają być stabilne, spokojne i skanowalne.
- Usunąć lub mocno przyciemnić dekoracyjne gradienty tła.
- Rozdzielić primitive controls od layoutu:
  - `FormToggle` nie importuje `ContentBox`,
  - sekcje ustawień używają lekkich group/field layoutów zamiast mini-kart.
- Ograniczyć opisy pod polami:
  - opis tylko dla ryzyka, restartu, trwałego zapisu, nieoczywistego zakresu,
  - brak duplikacji etykiety w opisie.
- Ujednolicić nagłówki kart i actions.
- Sprawdzić teksty PL/EN pod kątem długości i braku overlapu.

Akceptacja:

- Matrix LED, Power, Wi-Fi STA/AP, Notifications i Airmouse używają tego samego wzorca karty ustawień.
- Na mobile 360 px teksty nie wypadają z przycisków i nie nachodzą na kontrolki.
- Nie powstają nowe skróty/tabs w menu.

Checkpoint:

- Playwright screenshot desktop + mobile dla głównych ekranów ustawień
- `npm run check`
- patch `docs/refactor-checkpoints/phase-03-ui-system.patch`

### Etap 4 - Ładowanie Svelte i wydajność na urządzeniu

Cel: utrzymać single-bundle tylko jeśli pomiary potwierdzą, że to najlepszy trade-off dla firmware.

Zakres:

- Utrzymać baseline:
  - Client JS gzip: 316.72 KB / 400 KB,
  - CSS gzip: 28.15 KB / 50 KB,
  - total build: 1.25 MB / 1.5 MB.
- Uruchamiać `npm run build:analyze` po większych zmianach i zapisywać `stats.html` jako lokalny artefakt, nie commitować domyślnie.
- Przetestować eksperymentalny build `bundleStrategy: split` na branchu technicznym:
  - czy LittleFS i `viteLittleFS` obsłużą wiele plików,
  - czy firmware serwuje poprawne content-type i gzip,
  - czy realny first load na urządzeniu jest lepszy.
- Jeśli single zostaje:
  - usuwać martwy kod i zależności,
  - ograniczyć liczbę aktywnych DaisyUI themes, jeśli CSS zacznie rosnąć,
  - nie inwestować w dynamic import jako code splitting, bo single bundle zniweluje korzyść.
- Przetestować `data-sveltekit-preload-data="tap"` zamiast `hover` dla menu na urządzeniu.
- Dodać proste native performance marks:
  - app boot,
  - auth ready,
  - first system status,
  - first page interactive.
- Dodać raport z Chrome DevTools/Lighthouse w preview build, nie dev mode.

Akceptacja:

- Build dalej przechodzi size budget.
- Planowana zmiana bundlingu ma pomiar z urządzenia albo zostaje odrzucona.
- Brak nowego waterfallu API przy wejściu na Matrix LED.

Checkpoint:

- `npm run build`
- `node ./scripts/check-build-size.mjs`
- Lighthouse/Performance raport lokalny
- patch `docs/refactor-checkpoints/phase-04-performance.patch`

### Etap 5 - Dostępność, responsywność i bezpieczeństwo interakcji

Cel: ekran ustawień jest obsługiwalny klawiaturą, screen readerem i na mobile.

Zakres:

- Dodać `@axe-core/playwright` albo równoważny test a11y dla głównych ekranów.
- Upewnić się, że icon-only buttons mają `aria-label` i tooltip/title.
- Dodać dirty-state guard przy opuszczaniu strony z niezapisanymi zmianami.
- Dodać testy keyboard navigation dla modali i save actions.
- Sprawdzić `pointer-events-none` przy wyłączonych efektach: użytkownik musi nadal widzieć, dlaczego pola są nieaktywne, a screen reader nie powinien trafić w niespójny stan.

Akceptacja:

- E2E mobile/desktop nie pokazuje overlapów.
- A11y smoke test przechodzi dla Dashboard, Matrix LED, Wi-Fi, Notifications, Power.
- Unsaved changes są chronione przed przypadkową nawigacją.

Checkpoint:

- `npm run test:e2e`
- a11y smoke
- patch `docs/refactor-checkpoints/phase-05-a11y-responsive.patch`

### Etap 6 - Tooling i CI quality pipeline

Cel: narzędzia mają wymuszać jakość, a nie tylko istnieć w package.json.

Zakres:

- Dodać skrypt `quality:frontend`:
  - `npm run lint`,
  - `npm run check`,
  - `npm run test:run`,
  - `npm run deps:check`,
  - `npm run unused:check`,
  - `npm run build`,
  - `node ./scripts/check-build-size.mjs`.
- Dodać szybki `quality:frontend:fast` dla codziennej pracy bez pełnego builda/e2e.
- Wprowadzić regułę PR: nie powiększać dependency-cruiser baseline'u.
- Dodać test kontraktu settings cards: każda karta z edytowalnymi polami ma jawny Save albo jest oznaczona jako read-only/status.
- Ustawić Playwright trace/screenshot only-on-failure.
- Rozważyć Svelte/Vite inspector tylko w dev, bez wpływu na produkcję.

Akceptacja:

- Jedna komenda daje wiarygodny wynik gotowości frontendu.
- CI publikuje size summary i screenshoty/trace przy porażce.
- Baseline dependency-cruiser jest redukowany etapami.

Checkpoint:

- `npm run quality:frontend`
- patch `docs/refactor-checkpoints/phase-06-tooling-ci.patch`

### Etap 7 - Dokumentacja i release polish

Cel: dokumentacja, screenshoty i zachowanie aplikacji są zgodne.

Zakres:

- Zaktualizować screenshoty Matrix LED po refaktorze.
- Opisać semantykę zapisu: draft, Save, Reset, natychmiastowe Apply.
- Dodać krótką notkę operatorską dla Matrix physical menu.
- Upewnić się, że nazwa produktu/branding są spójne w screenshotach i UI.
- Dodać changelog UX: co zmieniło się dla użytkownika, bez technicznego żargonu.

Akceptacja:

- User guide pokazuje aktualny ekran i aktualny przepływ zapisu.
- Brak rozbieżności "PlantCare" vs "MatrixHub" w nowych materiałach.

Checkpoint:

- Screenshot diff zaakceptowany.
- patch `docs/refactor-checkpoints/phase-07-docs-release.patch`

## Kolejność napraw priorytetowych

1. Naprawić lint/Prettier, bo to blokuje zaufanie do pipeline'u.
2. Naprawić Matrix autosave retry loop, bo to może obciążać urządzenie.
3. Ustalić i wdrożyć jeden kontrakt zapisu settings cards.
4. Uporządkować Matrix LED: Save na kartach, modal ikon, `menu_enabled`.
5. Uspokoić visual system i usunąć nested-card pattern.
6. Dopiero potem mierzyć i ewentualnie zmieniać bundling/preload.

## Źródła techniczne

- SvelteKit Performance: https://svelte.dev/docs/kit/performance
- SvelteKit Configuration / `bundleStrategy`: https://svelte.dev/docs/kit/configuration
- SvelteKit Link options / preload: https://svelte.dev/docs/kit/link-options
- Svelte `$effect`: https://svelte.dev/docs/svelte/%24effect
- Svelte `$derived`: https://svelte.dev/docs/svelte/%24derived
