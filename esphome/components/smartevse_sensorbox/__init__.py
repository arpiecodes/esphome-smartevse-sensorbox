import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ADDRESS, CONF_ID

CONF_USE_CT_READINGS = "use_ct_readings"
CONF_CT_WIRES = "ct_wires"
CONF_CT_ROTATION = "ct_rotation"
CONF_P1_SENSORS = "p1_sensors"
CONF_P1_SENSOR_DSMR_VERSION = "dsmr_version"
CONF_P1_SENSOR_VOLTAGE_PHASE_1 = "voltage_phase_1"
CONF_P1_SENSOR_VOLTAGE_PHASE_2 = "voltage_phase_2"
CONF_P1_SENSOR_VOLTAGE_PHASE_3 = "voltage_phase_3"
CONF_P1_SENSOR_POWER_CONSUMED_PHASE_1 = "power_consumed_phase_1"
CONF_P1_SENSOR_POWER_CONSUMED_PHASE_2 = "power_consumed_phase_2"
CONF_P1_SENSOR_POWER_CONSUMED_PHASE_3 = "power_consumed_phase_3"
CONF_P1_SENSOR_POWER_PRODUCED_PHASE_1 = "power_produced_phase_1"
CONF_P1_SENSOR_POWER_PRODUCED_PHASE_2 = "power_produced_phase_2"
CONF_P1_SENSOR_POWER_PRODUCED_PHASE_3 = "power_produced_phase_3"

smart_evse_sensorbox_ns = cg.esphome_ns.namespace("smartevse_sensorbox")
SmartEVSESensorboxDeviceComponent = smart_evse_sensorbox_ns.class_("SmartEVSESensorbox", cg.Component)

CTWires = smart_evse_sensorbox_ns.enum("CT_WIRES")
CT_WIRES = {
    "4WIRE": 0,
    "3WIRE": 2,
}

CTRotation = smart_evse_sensorbox_ns.enum("CT_ROTATION")
CT_ROTATION = {
    "CW": 0,
    "CCW": 1,
}

sensor_ns = cg.esphome_ns.namespace("sensor")
SensorComponent = sensor_ns.class_("Sensor", cg.Component)

text_sensor_ns = cg.esphome_ns.namespace("text_sensor")
TextSensorComponent = text_sensor_ns.class_("TextSensor", cg.Component)

DEPENDENCIES = ["modbus_server"]

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SmartEVSESensorboxDeviceComponent),
            cv.Optional(CONF_USE_CT_READINGS): cv.boolean,
            cv.Optional(CONF_CT_WIRES, default="4WIRE"): cv.enum(
                CT_WIRES, upper=True
            ),
            cv.Optional(CONF_CT_ROTATION, default="CW"): cv.enum(
                CT_ROTATION, upper=True
            ),
            cv.Optional(CONF_P1_SENSORS): cv.Schema(
                {
                    cv.Required(CONF_P1_SENSOR_DSMR_VERSION): cv.use_id(TextSensorComponent),
                    cv.Required(CONF_P1_SENSOR_POWER_CONSUMED_PHASE_1): cv.use_id(SensorComponent),
                    cv.Required(CONF_P1_SENSOR_POWER_CONSUMED_PHASE_2): cv.use_id(SensorComponent),
                    cv.Required(CONF_P1_SENSOR_POWER_CONSUMED_PHASE_3): cv.use_id(SensorComponent),
                    cv.Required(CONF_P1_SENSOR_VOLTAGE_PHASE_1): cv.use_id(SensorComponent),
                    cv.Required(CONF_P1_SENSOR_VOLTAGE_PHASE_2): cv.use_id(SensorComponent),
                    cv.Required(CONF_P1_SENSOR_VOLTAGE_PHASE_3): cv.use_id(SensorComponent),
                    cv.Optional(CONF_P1_SENSOR_POWER_PRODUCED_PHASE_1): cv.use_id(SensorComponent),
                    cv.Optional(CONF_P1_SENSOR_POWER_PRODUCED_PHASE_2): cv.use_id(SensorComponent),
                    cv.Optional(CONF_P1_SENSOR_POWER_PRODUCED_PHASE_3): cv.use_id(SensorComponent),
                }
            )
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

MULTI_CONF = False
CODEOWNERS = ["@synegic"]


async def to_code(config):
    id = config[CONF_ID]
    sensorbox = cg.new_Pvariable(id)

    uart = await cg.get_variable(config["uart_id"])
    cg.add(sensorbox.set_uart_parent(uart))

    cg.add(sensorbox.use_ct_readings(config[CONF_USE_CT_READINGS]))
    cg.add(sensorbox.set_ct_rotation(config[CONF_CT_ROTATION]))
    cg.add(sensorbox.set_ct_wire(config[CONF_CT_WIRES]))

    if CONF_P1_SENSORS in config:
        conf_p1_sensors = config.get(CONF_P1_SENSORS)

        cg.add(sensorbox.set_p1_version_sensor(await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_DSMR_VERSION])))

        cg.add(sensorbox.add_p1_value_sensor(1, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_POWER_CONSUMED_PHASE_1])))
        cg.add(sensorbox.add_p1_value_sensor(2, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_POWER_CONSUMED_PHASE_2])))
        cg.add(sensorbox.add_p1_value_sensor(3, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_POWER_CONSUMED_PHASE_3])))
        cg.add(sensorbox.add_p1_value_sensor(4, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_VOLTAGE_PHASE_1])))
        cg.add(sensorbox.add_p1_value_sensor(5, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_VOLTAGE_PHASE_2])))
        cg.add(sensorbox.add_p1_value_sensor(6, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_VOLTAGE_PHASE_3])))

        if CONF_P1_SENSOR_POWER_PRODUCED_PHASE_1 in conf_p1_sensors:
            cg.add(sensorbox.add_p1_value_sensor(7, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_POWER_PRODUCED_PHASE_1])))

        if CONF_P1_SENSOR_POWER_PRODUCED_PHASE_2 in conf_p1_sensors:
            cg.add(sensorbox.add_p1_value_sensor(8, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_POWER_PRODUCED_PHASE_2])))

        if CONF_P1_SENSOR_POWER_PRODUCED_PHASE_3 in conf_p1_sensors:
            cg.add(sensorbox.add_p1_value_sensor(9, await cg.get_variable(conf_p1_sensors[CONF_P1_SENSOR_POWER_PRODUCED_PHASE_3])))


    await cg.register_component(sensorbox, config)

    return
