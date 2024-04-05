char descriptor[] = "\
  {\
  \"name\": \"TCS II Thermal Stimulator\",\
  \"device_id\": \"89uZDic6fZK0ReWTaFuP\",\
  \"device_type\": \"TCS_RaspberryPi_v1\",\
  \"data_to_report\": {\
    \"timestamp\": \"uint32_t_le\",\
    \"temperature\": \"uint32_t_le\"\
  },\
  \"data_to_control\": {\
    \"serial\": \"string\"\
  },\
  \"report_pack_order\": {\
    \"packed\": true,\
    \"order\":  [\"timestamp\", \"temperature\"]\
  },\
  \"control_pack_order\": \"serial\",\
  \"visual_report\": {\
    \"temperature\": \"line_chart\",\
    \"rolling_time\": 1.5,\
    \"expand_packed\": true,\
    \"data_rate\": 100\
  },\
  \"visual_control\": {\
    \"serial\": \"static\"\
  }\
}";
