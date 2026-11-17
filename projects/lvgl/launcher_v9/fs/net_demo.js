/*
 * HTTP/HTTPS demo for the JavaScript network bindings.
 *
 * 1. Set HOST to the address of the PC running the test server below.
 * 2. Rebuild the littlefs image and flash it to usr_config.
 * 3. Run from the console:
 *      js run /apps/net_demo.js
 *
 * Test server (must accept both GET and POST):
 *   python3 -m http.server 8000 answers GET only and rejects POST with 501,
 *   so use a small handler that also implements do_POST for the POST case.
 *
 * The HTTPS case talks to example.com and is only available when the AP
 * firmware is built with CONFIG_WEBCLIENT_TLS (certificates are not verified).
 */

var HOST = '192.168.1.10';
var PORT = 8000;

var BASE = 'http://' + HOST + ':' + PORT;

function show(tag, r) {
  console.log('[' + tag + '] status=' + r.status + ' length=' + r.length);
  console.log('[' + tag + '] body=' + r.body);
}

function fail(tag) {
  return function (e) {
    console.log('[' + tag + '] error: ' + e);
  };
}

var st = wifi.status();
console.log('wifi connected=' + st.connected + ' ip=' + st.ip + ' ssid=' + st.ssid);
console.log('request base ' + BASE);

http.get(BASE + '/').then(function (r) {
  show('GET', r);
}, fail('GET'));

http.post(BASE + '/echo', 'hello from bk7259', { contentType: 'text/plain' })
  .then(function (r) {
    show('POST', r);
  }, fail('POST'));

http.get('https://example.com/').then(function (r) {
  console.log('[HTTPS] status=' + r.status + ' length=' + r.length);
  console.log('[HTTPS] body head=' + r.body.substring(0, 64));
}, fail('HTTPS'));
