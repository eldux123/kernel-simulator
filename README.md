# 🖥️ Kernel Simulator

Simulador completo de núcleo de sistema operativo desarrollado en C++17 que integra gestión de memoria, planificación de procesos, sincronización y E/S.

## 🚀 Características

- **Gestión de Procesos**: Crear, suspender, reanudar y terminar procesos con soporte de hilos
- **Planificación**: Round Robin y SJF (Shortest Job First)
- **Memoria Virtual**: FIFO, LRU y PFF (Page Fault Frequency)
- **Heap Allocator**: Buddy System con coalescencia automática
- **Sincronización**: Semáforos, Productor-Consumidor, Filósofos, Lectores-Escritores
- **Planificación de Disco**: FCFS, SSTF y SCAN
- **E/S**: Cola de prioridad para 3 dispositivos (Impresora, Disco, Red)
- **CLI**: Interfaz de 19 opciones organizadas

## 📁 Estructura del Proyecto

```
kernel-simulator/
├── kernel-sim/          # Punto de entrada (Main.cpp)
├── cli/                 # Interfaz de línea de comandos
├── modules/
│   ├── cpu/            # Procesos, planificación y sincronización
│   ├── mem/            # Memoria virtual y heap allocator
│   ├── disk/           # Planificación de disco
│   └── io/             # Gestión de E/S
├── docs/               # Documentación y scripts de prueba (16 archivos)
└── build/              # Archivos objeto compilados
```

## 🔧 Compilación

### Opción 1: Script automático
```powershell
.\compile.ps1
```

### Opción 2: VS Code
Presiona `F5` o `Ctrl+F5` en `Main.cpp`

### Opción 3: Manual
```powershell
g++ -std=c++17 -c modules/cpu/*.cpp -o build/
g++ -std=c++17 -c modules/mem/*.cpp -o build/
g++ -std=c++17 -c modules/disk/*.cpp -o build/
g++ -std=c++17 -c modules/io/*.cpp -o build/
g++ -std=c++17 -c cli/CLI.cpp -o build/CLI.o
g++ -std=c++17 kernel-sim/Main.cpp build/*.o -o kernel-sim.exe
```

## ▶️ Ejecución

```powershell
.\kernel-sim.exe
```

## 📋 Menú Principal

```
GESTIÓN DE PROCESOS (1-5)
- Crear procesos (normal, productor, consumidor)
- Listar y terminar procesos

GESTIÓN DE HILOS (14-15)
- Crear y mostrar hilos

EJECUCIÓN (6-7)
- Avanzar por ticks (simulación)

REPORTES (8-11)
- Estadísticas del sistema
- Estado de memoria y buffer

CONFIGURACIÓN (12-13)
- Ajustar memoria y algoritmos

HEAP ALLOCATOR (16-19)
- Asignar/liberar memoria dinámica
- Estadísticas de fragmentación
```

## 📊 Algoritmos Implementados

| Módulo | Algoritmos |
|--------|-----------|
| Memoria | FIFO, LRU, **PFF** (avanzado) |
| Planificación | Round Robin, SJF |
| Disco | FCFS, SSTF, SCAN |
| Heap | Buddy System |
| Sincronización | Semáforos, Prod-Cons, Filósofos, Lect-Escr |

## 📖 Documentación

- **Informe Técnico**: `INFORME_TECNICO_CONSOLIDADO.txt` (800+ líneas)
- **Arquitectura**: `docs/ARQUITECTURA.md`
- **Diseño**: `docs/design.md`
- **Scripts de Prueba**: `docs/*.txt` (16 archivos)
- **Verificación**: `docs/VERIFICACION_FINAL_REQUISITOS.md`

## 🧪 Scripts de Prueba

Todos los scripts están en `docs/`:
- `mem_*.txt` - Pruebas de memoria (FIFO, LRU, PFF)
- `disk_*.txt` - Pruebas de disco (FCFS, SSTF, SCAN)
- `proc_*.txt` - Pruebas de procesos y sincronización

## 📈 Métricas del Proyecto

- **Código**: 2,628 líneas
- **Documentación**: 2,134 líneas
- **Módulos**: 4 (CPU, MEM, DISK, IO)
- **Clases**: 14
- **Algoritmos**: 13
- **Problemas de sincronización**: 4

## 🎯 Requisitos

- **Compilador**: g++ con soporte C++17
- **SO**: Windows (MinGW/MSYS2) o Linux
- **Herramientas**: Git, VS Code (opcional)

## 👨‍💻 Desarrollo

**Arquitectura**: Modular con separación de responsabilidades  
**Estilo**: Orientado a objetos  
**Estándares**: C++17, documentación Doxygen  

## 📝 Licencia

Proyecto académico - Universidad 2025

---

⭐ **Proyecto completo y funcional** - Cumple 100% de requisitos + características extras
