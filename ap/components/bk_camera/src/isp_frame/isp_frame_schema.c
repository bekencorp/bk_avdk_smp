#define ISP_FRAME_FORM_JSON \
    "{\"id\":\"isp-wifi-config\",\"title\":\"ISP Capture Config\"," \
    "\"groups\":[],\"submit\":{\"label\":\"Apply\",\"method\":\"set_config\",\"build\":\"tree\"}}"

#define ISP_FRAME_RATE_CTRL_SCHEMA_JSON \
    "{\"supported\":[],\"note\":\"No project-specific ISP frame schema is registered.\"}"

const char *isp_frame_session_get_form_json(void) __attribute__((weak));
const char *isp_frame_session_get_form_json(void)
{
    return ISP_FRAME_FORM_JSON;
}

const char *isp_frame_session_get_rate_ctrl_schema_json(void) __attribute__((weak));
const char *isp_frame_session_get_rate_ctrl_schema_json(void)
{
    return ISP_FRAME_RATE_CTRL_SCHEMA_JSON;
}
