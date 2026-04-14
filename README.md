# silverhand_arm_control

Пакет ROS 2 Jazzy для слоя управления рукой SilverHand.

Пакет:
- `silverhand_arm_control`

В этом репозитории намеренно оставлены только нижний и средний слои управления:
- `ros2_control`
- hardware interface
- controller bringup

Геометрия робота, меши и просмотр модели в RViz теперь живут в `silverhand_arm_model`.
Верхнеуровневое планирование остаётся в `silverhand_system_bringup`.

## Требования

- Ubuntu `24.04`
- ROS 2 `Jazzy`
- `libcxxcanard`, клонированный в тот же workspace

Установить нужные ROS-пакеты:

```bash
sudo apt-get update
sudo apt-get install -y \
  ros-jazzy-ros2-control \
  ros-jazzy-controller-manager \
  ros-jazzy-joint-trajectory-controller \
  ros-jazzy-joint-state-broadcaster \
  ros-jazzy-robot-state-publisher \
  ros-jazzy-hardware-interface \
  ros-jazzy-pluginlib \
  ros-jazzy-rclcpp \
  ros-jazzy-rclcpp-lifecycle \
  ros-jazzy-xacro \
  ros-jazzy-urdf
```

## Клонирование

Клонируйте в workspace, где уже есть `libcxxcanard`:

```bash
cd ~/silver_ws/src
git clone https://github.com/VB-Industrial/libcxxcanard.git
git clone <repo-url>
```

`libcxxcanard` должен лежать рядом с пакетом:

```bash
~/silver_ws/src/libcxxcanard
```

## Примечания

- `libcxxcanard` - отдельная зависимость workspace.
- По умолчанию bringup использует `use_mock_hardware:=true`, поэтому стек можно поднять без CAN-железа.
- Описание управления включает `silverhand_arm_model` и добавляет системный блок `ros2_control`.

## Структура workspace

Пакет можно собирать отдельно:

```bash
~/projects/silverhand_arm_control
```

или вместе с верхнеуровневыми репозиториями в общем workspace:

```bash
~/silver_ws/src/silverhand_arm_control
~/silver_ws/src/silverhand_arm_model
~/silver_ws/src/silverhand_system_bringup
```

## Сборка

Отдельная сборка:

```bash
cd ~/projects/silverhand_arm_control
source /opt/ros/jazzy/setup.bash
colcon build
source install/setup.bash
```

Общий workspace:

```bash
cd ~/silver_ws
source /opt/ros/jazzy/setup.bash
colcon build
source install/setup.bash
```

## Проверка пакетов

```bash
ros2 pkg list | rg silverhand
```

Ожидаемый пакет из этого репозитория:
- `silverhand_arm_control`

## Запуск

Mock hardware:

```bash
ros2 launch silverhand_arm_control silverhand_arm_bringup.launch.py
```

Эквивалентный явный mock-запуск:

```bash
ros2 launch silverhand_arm_control silverhand_arm_bringup.launch.py use_mock_hardware:=true
```

Real hardware:

```bash
ros2 launch silverhand_arm_control silverhand_arm_bringup.launch.py use_mock_hardware:=false can_iface:=can0 node_id:=100
```

## Вспомогательные скрипты

Для типового запуска есть helper-скрипты:

```bash
cd ~/silver_ws/src/silverhand_arm_control
./scripts/start_arm_mock.sh
./scripts/start_arm_real.sh
```

Поддерживаемые переменные окружения:

- `ROS_WS` — путь к workspace, по умолчанию определяется как `~/silver_ws`
- `ROS_DISTRO` — по умолчанию `jazzy`
- `SILVERHAND_ARM_CAN_IFACE` — по умолчанию `can0`
- `SILVERHAND_ARM_NODE_ID` — по умолчанию `100`

## systemd

Шаблон systemd-сервиса:

- `systemd/system/silverhand-arm-control@.service`

Экземпляры:

- `mock`
- `real`

Установка:

```bash
sudo install -Dm644 systemd/system/silverhand-arm-control@.service /etc/systemd/system/silverhand-arm-control@.service
sudo systemctl daemon-reload
```

Запуск:

```bash
sudo systemctl enable --now silverhand-arm-control@mock.service
```

или:

```bash
sudo systemctl enable --now silverhand-arm-control@real.service
```

Автозапуск без логина не нужен: system-сервис стартует без пользовательской сессии.

Полезные команды:

```bash
systemctl status silverhand-arm-control@mock.service
journalctl -u silverhand-arm-control@mock.service -f
sudo systemctl restart silverhand-arm-control@mock.service
sudo systemctl disable --now silverhand-arm-control@mock.service
```

## Параметры

Аргументы запуска:
- `use_mock_hardware`
- `can_iface`
- `node_id`

Значения по умолчанию:
- `use_mock_hardware:=true`
- `can_iface:=can0`
- `node_id:=100`

## Интеграция

Верхнеуровневые пакеты вроде `silverhand_system_bringup` зависят от:
- `silverhand_arm_model`
- `silverhand_arm_control`

Если пакеты собираются отдельно, порядок `source` должен быть таким:

```bash
source /opt/ros/jazzy/setup.bash
source ~/silver_ws/install/setup.bash
```
