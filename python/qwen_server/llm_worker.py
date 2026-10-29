from queue import Queue
from threading import Thread, Event, Lock

import torch
from transformers import AutoModelForCausalLM, AutoTokenizer


# One text-generation request.
class LLMRequest:
    def __init__(self, text):
        self.text = text
        self.reply = ""
        self.error = None
        self.finished = Event()


class LLMWorker:
    def __init__(self):
        self.model_id = "Qwen/Qwen3-0.6B"

        self.model = None
        self.tokenizer = None

        self.request_queue = Queue()
        self.worker_thread = None
        self.running = False
        self.state_lock = Lock()

        self.system_prompt = (
            "You are Lily, a friendly virtual character. "
            "Reply in English using one or two short sentences. "
            "Use natural spoken language without Markdown. "
            "If you do not know something, say so."
        )

    # Load the model once and start the background thread.
    def start(self):
        with self.state_lock:
            if self.running:
                return

            self.loadModel()

            self.running = True
            self.worker_thread = Thread(
                target=self.workerLoop,
                name="LLMWorker",
                daemon=True
            )
            self.worker_thread.start()

        print("LLM worker started.")

    # Finish queued requests, then stop.
    def stop(self):
        with self.state_lock:
            if not self.running:
                return

            self.running = False
            self.request_queue.put(None)
            thread = self.worker_thread

        if thread is not None:
            thread.join()

        print("LLM worker stopped.")

    # Submit text and return an object holding the future result.
    def submit(self, text):
        if not isinstance(text, str) or not text.strip():
            raise ValueError("Text must be a non-empty string")

        with self.state_lock:
            if not self.running:
                raise RuntimeError("LLM worker is not running")

            request = LLMRequest(text.strip())
            self.request_queue.put(request)

        return request

    def loadModel(self):
        if not torch.cuda.is_available():
            raise RuntimeError("No CUDA available for LLM")

        print("Loading Qwen3 LLM...")

        self.tokenizer = AutoTokenizer.from_pretrained(
            self.model_id
        )

        self.model = AutoModelForCausalLM.from_pretrained(
            self.model_id,
            torch_dtype=torch.bfloat16,
            attn_implementation="sdpa"
        ).to("cuda:0")

        self.model.eval()

        print("Qwen3 LLM loaded.")

    def workerLoop(self):
        while True:
            request = self.request_queue.get()

            try:
                if request is None:
                    return

                try:
                    self.generate(request)

                except Exception as error:
                    request.error = error

                finally:
                    # Always notify the caller, including on failure.
                    request.finished.set()

            finally:
                self.request_queue.task_done()

    def generate(self, request):
        messages = [
            {
                "role": "system",
                "content": self.system_prompt
            },
            {
                "role": "user",
                "content": request.text
            }
        ]

        # Disable thinking for direct spoken replies. arrange the messages in a chat format and add a generation prompt.
        prompt = self.tokenizer.apply_chat_template(
            messages,
            tokenize=False,
            add_generation_prompt=True,
            enable_thinking=False
        )
        # transfer the prompt to token and put into GPU and generate the response.
        #input include inputs_id and attention_mask, 
        #and the shape of inputs_id is [1, seq_len], the shape of attention_mask is [1, seq_len]
        inputs = self.tokenizer(
            prompt,
            return_tensors="pt"
        ).to("cuda:0")

        with torch.inference_mode():
            outputs = self.model.generate(
                **inputs,
                max_new_tokens=100,
                do_sample=True,
                temperature=0.7,
                top_p=0.8,
                top_k=20,
                pad_token_id=self.tokenizer.eos_token_id
            )

        # Decode only the newly generated answer.
        generated_tokens = outputs[
            0, inputs["input_ids"].shape[1]:
        ]

        request.reply = self.tokenizer.decode(
            generated_tokens,
            skip_special_tokens=True
        ).strip()

        if not request.reply:
            raise RuntimeError("LLM returned an empty reply")

        print(f"Reply: {request.reply}", flush=True)