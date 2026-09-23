//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/bk_hardware_ram.h>

#define TAG "bk_mem_sram"
#define LOGI(...) BK_LOGI(TAG, __VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

void *bk_get_isp_flexa_buffer(uint32_t size)
{
    void *ptr = os_sram_malloc(size);

    if (ptr == NULL) {
        LOGW("%s: sram_malloc failed, use hsram_malloc", __func__);
        ptr = hsram_malloc(size);
    }
    return ptr;
}

void *bk_get_gpu_flexa_buffer(uint32_t size)
{
    void *ptr = os_sram_malloc(size);

    if (ptr == NULL) {
        LOGW("%s: sram_malloc failed, use hsram_malloc", __func__);
        ptr = hsram_malloc(size);
    }
    return ptr;
}

void *bk_get_gpu_output_buffer(uint32_t size)
{
    void *ptr = os_sram_malloc(size);

    if (ptr == NULL) {
        LOGW("%s: sram_malloc failed, use hsram_malloc", __func__);
        ptr = hsram_malloc(size);
    }
    return ptr;
}

void *bk_get_dpu_buffer(uint32_t size)
{
    return hsram_malloc(size);
}
