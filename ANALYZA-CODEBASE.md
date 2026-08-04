# Analýza codebase GLibpp

> **Stav k commitu:** `0adc39a` (`0adc39a41bcbd3b62d71850af67ea23e3d6ba6b4`) — *Claude skills*, 2026-08-04, větev `main`
> **Datum analýzy:** 2026-08-04
> **Rozsah:** kompletní čtení všech podstatných zdrojových souborů (~6 600 řádků pod `GLibpp/src/`)

---

# Celkové zhodnocení

Jádro projektu je architektonicky **nadprůměrně dobře navržené** — vláknový model, handle systém a resource management jsou promyšlené a správně implementované věci, které v hobby enginech běžně chybí. Kolem tohoto zdravého jádra se ale nabalila vrstva mrtvého kódu, experimentů a nedotažených míst, která zhoršuje čitelnost a nese pár skutečných chyb. Odhadovaný poměr: ~60 % kódu je kvalitní jádro, ~25 % je funkční ale nedotažené demo/scaffolding, ~15 % je mrtvý kód.

## Co je silné (a čeho se držet)

- **Vláknový model** — `ZeroAllocTripleBuffer` je korektně napsaný SPSC (správné acquire/release ordering, cache-line padding), interpolace mezi logickými stavy s timestampy z logického času (ne wall-clock) je řešení, které eliminuje jitter scheduleru u zdroje. Tohle je nejcennější část enginu.
- **Handle systém** — `StableRegistry` s generacemi, default-invalid handly, typová bezpečnost přes vnořený typ (`ResourceHandles.h`). Čisté.
- **Separace canonical storage vs. residency** — `ResourceManager` (App vlastní data) vs. `RegistryDIB` (backend má privátní kopie s offsety) přesně kopíruje flow budoucího GL backendu (register/update/draw-by-handle). Příprava na GL je reálná, ne jen deklarovaná.
- **Vynucené invarianty** — `static_assert(std::is_trivially_copyable_v<Scene>)` je přesně ten typ ochrany, který udrží zero-alloc publish i při dalším vývoji. Asserty na `freeze()` kontrakt taktéž.
- **Input** — double-buffering klávesnice s jasným kontraktem (raw schránka z window vlákna, překlopení v logickém kroku) je správný návrh.
- **Komentáře vysvětlují „proč"**, ne „co" — vláknové kontrakty jsou zdokumentované přímo u kódu.

---

# Skutečné chyby (opravit)

1. **`Mtx4::inverse()` — mrtvý fallback pro singulární matici** (`Mtx4.cpp:278-291`). Když `det == 0`, nastaví se `*this` na nulovou matici, ale kód pokračuje: `invDet = 1.0f / det` → inf, a `(*this) = inv` na konci **fallback přepíše maticí plnou inf/NaN**. Fallback je fakticky nedosažitelný.

2. **`Mtx4::setIdentity()` je rozbité** (`Mtx4.cpp:128-137`). Nemodifikuje `this` — vrací referenci na **sdílenou mutable statickou matici**. Kdokoli by výsledek zmutoval (a mutující buildery jsou zde idiom!), poškodí „identitu" pro celý proces. Nikde se nevolá, takže to zatím nic nerozbilo — o důvod víc to smazat nebo opravit hned.

3. **`static std::vector<float> viewPos` v `rasterizeMesh`** (`DeviceDIB.h:256`). Function-local static = skrytý globální stav sdílený všemi instancemi device. Sousední `floatBuffer` je přitom member — nekonzistence naznačuje, že to je pozůstatek. Dnes to funguje (jedno render vlákno), ale je to nášlapná mina pro druhý device/thread. Přesunout na member.

4. **Scene.h není self-contained** — používá `CarTransformation`, kterou nedefinuje ani neincluduje. Kompiluje se jen proto, že `App.h` definuje `CarTransformation` **před** `#include "Scene.h"` uprostřed souboru (`App.h:188`). Kterýkoli TU, který by includnul `Renderer.h` nebo `Scene.h` samostatně, se nezkompiluje. Tohle je největší strukturální fragilita v projektu.

5. **`WheelTransformation::rollAngle` roste bez omezení** (`App.h:21-23`). Po delší jízdě ztratí float přesnost (u hodnot ~10⁵ je rozlišení už jen ~0.01 rad) → viditelný jitter rotace kol, který interpolace nezachrání. Wrapovat do [0, 2π) — a pak pozor, `Lerp` úhlů musí být wrap-aware.

6. **`main.cpp:18` — `catch (std::runtime_error error)` by value** (slicing + kopie). Má být `const std::exception&`. Mimochodem `main.cpp:15` žádá `DISPLAY1`, zatímco CLAUDE.md tvrdí `DISPLAY2` — drift dokumentace.

7. **Formálně UB data race v `Keyboard::setKeyState`** (`Input.h:23-27`) — zápis do `bool` z window vlákna, čtení z logického (zde je to totéž vlákno, ale kontrakt v komentáři výslovně počítá s asynchronním zápisem). Komentář „na x86 je to atomické" je prakticky pravdivý, ale `std::array<std::atomic<bool>, 256>` by stál stejně a byl korektní. Podobně `ResizeRequest` může teoreticky utrhnout w/h při dvou rychlých `set()` po sobě — u resize eventů prakticky neškodné, ale stojí za komentář nebo spakování do jednoho `atomic<uint64_t>`.

---

# Architektonické problémy (předělat)

## 1. Renderer má hardcoded demo scénu — nejdůležitější refactor

`Renderer::renderFrame` (`Renderer.h:106-163`) jmenovitě zná `gridWave`, `carBody`, `wheel`, `icosphere`, `icrBeam` a volá `scene.car.getFrontLeft()` atd. **Renderer je tím svázaný s konkrétním demem** — každá nová scéna znamená editaci enginu. Navíc parametry vlny (60, 0.2f, 0.05f) jsou duplikované mezi `App::setupDemoResources` a `renderFrame` (komentář to přiznává), a Renderer kvůli tomu includuje `MeshFactory`.

**Jak:** do `Scene` přidat fixní pole draw itemů — `struct DrawItem { MeshInstanceHandle handle; Mtx4 world; }; std::array<DrawItem, N> items; uint32_t count;` — které naplní `updateLogic` (tam se ty world matice stejně počítají), a `renderFrame` degraduje na smyčku `for (…) drawInstance(ctx, item.handle, item.world)`. Scene zůstane trivially copyable, Slerp world matic už existuje (`Mtx4::Slerp` — dnes mrtvý, tady by ožil; anebo interpolovat dál na úrovni Car a draw list stavět až z interpolovaného stavu na render vlákně, což je čistší). Dynamickou vlnu vyjádřit obecně (např. flag/callback u meshe registrovaný v App), ať parametry žijí na jednom místě. **Tohle udělat před GL backendem** — jinak GL backend zdědí demo natvrdo zadrátované do smyčky.

## 2. App.h je god-header

610 řádků: `WheelTransformation` + `CarTransformation` (herní/fyzikální kód), pak uprostřed souboru blok includů (`App.h:188-199`), výběr backendu makrem, a třída App s ~90 řádky `updateLogic`, z nichž **zhruba polovina je mrtvá** (manipulace `matrixVehicle`, `matrixWheel01-04`, `scene.speed`, `scene.test`; duplicitní KEY_ENTER handler na řádcích 379 a 454). Navíc backend-macro mechanismus je napůl obejitý — member je natvrdo `Renderer<Render::DeviceDIB>` (`App.h:233`) kvůli IDE, takže přepnutí makra by ve skutečnosti backend nezměnilo.

**Jak:** vytáhnout `CarTransformation`/`WheelTransformation` do `src/App/CarTransformation.h` (přes `glib-add-file` skill), Scene.h ho includne → vyřeší se i fragilita č. 4 výše. Mrtvé větve v `updateLogic` smazat spolu s mrtvými poli Scene.

## 3. Scene nese ~550 B mrtvých dat kopírovaných 60× za sekundu

`modelMatrix/2/3`, `matrixVehicle`, `matrixWheel01-04`, `matrixSteer`, `speed`, `rotationSpeed`, `cameraSpeed`, `cameraRotationSpeed`, `test` (`Scene.h:27-46`) — 9 mrtvých matic + 5 floatů. Funkčně to nevadí (publish je memcpy), ale mate to čtenáře: není poznat, co je živý stav. Smazat.

## 4. Mrtvý kód je roztroušený všude

Ověřeno grepem, tohle nikdo nevolá / nikam nevede:

- `Backend/RenderCommand/` (5 hlaviček) a `Backend/Stencil/` (fakticky prázdné soubory) — pořád visí ve vcxproj
- `DoubleBuffer.h` (includován v Renderer.h, nepoužit), `drawTriangle_` (kopie drawTriangle), `drawStaticTestMesh*`, `Mtx4::Slerp` (zatím), `setIdentity`, `sleepIfIdle`, `waitUntilNextStep__`, `fastDiv`/`reciprocal`, `Material` (stub), zakomentovaný `render()` na konci App.h
- `Camera.h` obsahuje **dvě kompletní implementace** přepínané trikem `/* … /*/ … //*/` (`Camera.h:17-192` je neaktivní quaternionová verze). Neaktivní smazat — drží ji git.
- V `CarTransformation::run` mrtvé lokály `object`, `dRoll`; v `rollAllWheels` mrtvý `eps`.

Mazání je zadarmo a je to největší poměr přínos/riziko v celém seznamu.

## 5. Chybí testy — a tenhle kód je na ně ideální

Matematika (`Mtx4::inverse` — viz chyba č. 1, `Quaternion::Slerp`, `LookAt`/`Perspective`) a datové struktury (`TripleBuffer`, `StableRegistry`, `ZeroAllocStateHistory`) jsou čisté, deterministické, bez závislostí na Win32 — perfektní kandidáti. Chybu č. 1 by jednořádkový test odhalil okamžitě. **Jak:** druhý console projekt v solution (`GLibppTests`) s ručním mini-runnerem (žádný framework není potřeba — pár `assert` funkcí stačí), spouštěný před commitem (šel by přidat do `glib-commit` rituálu).

## 6. Nekonzistentní error handling a konvence

- `targetGetImpl` a `DeviceTargetDIB` ctor házejí výjimky; `drawMeshImpl`/`clearImpl` tiše returnují; registry používá asserty; `checkWindowInitialized` vrací bool, ale ve skutečnosti hází. Doporučený explicitní kontrakt: **výjimky jen v init fázi, asserty na programátorské chyby, tiché skipy jen v render hot-path** — a napsat ho do CLAUDE.md.
- `std::cout` roztroušený po celém kódu (i v hot path — „Zaskub" log je aspoň komentářem obhájený). Časem jednoduchá log fasáda.
- `Mtx4::Perspective` je GL-konvence (z ∈ −1..1), `Orthographic` D3D-konvence (z ∈ 0..1) — až se začne používat ortho nebo depth buffer, tohle se projeví jako „záhadný" bug.
- `Vec4` konfluje bod a směr (default w=1, `normalized()` dělí i w, `cross` w ignoruje, `operator+` sčítá w → 1+1=2). Kód to obchází disciplínou (w=0 u směrů), ale je to trvalý zdroj chyb. Dlouhodobě: `Vec3` + explicitní `toPoint()/toDir()`, krátkodobě aspoň zdokumentovat konvenci.
- `TimeManager` má `#include <immintrin.h>` **uvnitř těla třídy** (`TimeManager.h:235`) — funguje, ale patří nahoru; tři překrývající se konstruktory by šly sloučit.

## 7. Známé stuby brát vážně při plánování

Bez depth bufferu a bez clippingu (all-or-nothing frustum test per-triangle) je každé rozšíření scény riziko — větší ground plane = viditelné mizení trojúhelníků na okrajích. Depth buffer + aspoň near-plane clipping **změní API rasterizeru** (potřebuje interpolovat z, dělit trojúhelníky), takže je lepší je udělat před tím, než na rasterizer naváže víc kódu. Rasterizer sám má celočíselný `edgeInterp` bez subpixel přesnosti → „vertex crawling" při pomalém pohybu; to je akceptovatelné pro demo, ale při ladění stutteru to může mást měření.

---

# Doporučené pořadí prací

| # | Co | Proč v tomhle pořadí |
|---|----|----|
| 1 | Smazat mrtvý kód (Scene pole, updateLogic větve, Camera duplikát, RenderCommand/Stencil, drobnosti) | Zadarmo, zmenší kognitivní zátěž pro všechno další |
| 2 | Vytáhnout CarTransformation do vlastního headeru; opravit chyby 1, 2, 3, 5, 6 | Malé, izolované, odstraní fragilitu includů |
| 3 | Testovací projekt pro math + datastruct | Pojistka pro kroky 4–5 |
| 4 | Data-driven draw list ve Scene (zrušit hardcoded scénu v renderFrame) | Odemyká GL backend a nové scény; největší strukturální změna |
| 5 | Depth buffer + near-plane clipping | Před dalším růstem scén; mění rasterizer API |
| 6 | GL backend | Až na připravený, otestovaný základ |

**Shrnuto:** základy jsou solidní a směr (GL-shaped residency, handle systém, zero-alloc publish) je správný — projekt netrpí špatnou architekturou, ale **nedokončeným úklidem po evoluci**. Největší reálná rizika jsou fragilní include-order závislost Scene↔CarTransformation, hardcoded scéna v Rendereru a neexistence testů nad matematikou, kde už teď leží dvě skutečné chyby.
