#define H264E_STREAM_FORM_JSON \
    "{\"id\":\"h264e-stream-config\",\"title\":\"H264E Stream Encoder Config\"," \
    "\"groups\":[],\"submit\":{\"label\":\"Apply\",\"method\":\"set_config\",\"build\":\"tree\"}}"

#define H264E_STREAM_RATE_CTRL_SCHEMA_JSON \
    "{\"supported\":[],\"notExposed\":[],\"vcencAllFields\":[]}"

const char *h264e_stream_session_get_form_json(void) __attribute__((weak));
const char *h264e_stream_session_get_form_json(void)
{
    return H264E_STREAM_FORM_JSON;
}

const char *h264e_stream_session_get_rate_ctrl_schema_json(void) __attribute__((weak));
const char *h264e_stream_session_get_rate_ctrl_schema_json(void)
{
    return H264E_STREAM_RATE_CTRL_SCHEMA_JSON;
}
