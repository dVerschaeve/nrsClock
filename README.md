# nrsClock
Wemos D1 Mini Clock

Home Assistant configuration.yaml:
  - platform: mqtt_json
    name: "NRS Clock"
    state_topic: "stat/nrsClock/LED"
    command_topic: "cmnd/nrsClock/LED"
    brightness: true
    rgb: true
    effect: true
    effect_list: [CountDown, SnowSparkle,meteorRain, CylonBounce, Strobe]
    optimistic: false
    qos: 0

Trigger in Home Assistant via an Automation:
- service: mqtt.publish
      data_template:
        topic: "cmnd/nrsClock/LED"
        payload: "{\"state\":\"ON\",\"effect\":\"CountDown\"}"
