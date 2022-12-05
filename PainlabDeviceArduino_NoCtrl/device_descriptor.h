char descriptor[] = "\
  {\
  \"name\": \"Physical monitoring / Arduino\",\
  \"device_id\": \"42QkSmZhTlovVgDW89vJ\",\
  \"device_type\": \"Phys_Monitoring_Arduino_v1\",\
  \"data_to_report\": {\
    \"timestamp\": \"uint32_t_le\",\
    \"skin_conductance\": \"uint16_t_le\",\
    \"heart_rate\": \"uint16_t_le\",\
    \"electromyography\": \"uint16_t_le\"\
  },\
  \"data_to_control\": null,\
  \"report_pack_order\": {\
    \"packed\": true,\
    \"order\":  [\"timestamp\", \"skin_conductance\", \"heart_rate\", \"electromyography\"]\
  },\
  \"control_pack_order\": null,\
  \"visual_report\": {\
    \"skin_conductance\": \"line_chart\",\
    \"heart_rate\": \"line_chart\",\
    \"electromyography\": \"line_chart\",\
    \"data_rate\": 33.3\
  },\
  \"visual_control\": {}\
}";
