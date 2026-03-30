import argparse
import os
from dotenv import load_dotenv


DEFAULT_PROMPT = "Tell me a fun fact"


MODEL_CHOICES = [
    {
        "provider": "Gemini",
        "label": "Gemini 2.5 Flash Lite",
        "model": "gemini-2.5-flash-lite",
        "env_var": "GOOGLE_API_KEY",
        "base_url": "https://generativelanguage.googleapis.com/v1beta/openai/",
        "api_key": None,
    },
    {
        "provider": "Ollama",
        "label": "DeepSeek R1 1.5B",
        "model": "deepseek-r1:1.5b",
        "env_var": None,
        "base_url": "http://localhost:11434/v1",
        "api_key": "ollama",
    },
    {
        "provider": "Ollama",
        "label": "Llama 3.2 1B",
        "model": "llama3.2:1b",
        "env_var": None,
        "base_url": "http://localhost:11434/v1",
        "api_key": "ollama",
    },
    {
        "provider": "OpenAI",
        "label": "GPT-5 Nano",
        "model": "gpt-5-nano",
        "env_var": "OPENAI_API_KEY",
        "base_url": None,
        "api_key": None,
    },
]


def load_api_key(env_var):
    if env_var is None:
        return None
    return os.getenv(env_var)


def build_available_models():
    available = []

    for choice in sorted(MODEL_CHOICES, key=lambda item: (item["provider"], item["label"])):
        api_key = choice["api_key"] or load_api_key(choice["env_var"])

        if choice["env_var"] and not api_key:
            continue

        available.append({**choice, "resolved_api_key": api_key})

    return available


def print_api_key_status():
    openai_key = os.getenv("OPENAI_API_KEY")
    google_key = os.getenv("GOOGLE_API_KEY")

    if openai_key:
        print("OpenAI key found.")
    else:
        print("OpenAI key not found. OpenAI models will be hidden.")

    if google_key:
        print("Google key found.")
    else:
        print("Google key not found. Gemini models will be hidden.")

    print("Ollama models are shown if your local server is running on http://localhost:11434.")


def print_models(models):
    print("\nAvailable models:")
    for choice in models:
        print(f"  - {choice['model']}: [{choice['provider']}] {choice['label']}")


def build_parser():
    parser = argparse.ArgumentParser(description="Run a prompt against a selected LLM provider.")
    parser.add_argument(
        "--model",
        help="Model id to use, for example: gpt-5-nano, gemini-2.5-flash-lite, llama3.2:1b",
    )
    parser.add_argument(
        "--prompt",
        default=DEFAULT_PROMPT,
        help=f"Prompt to send to the model. Defaults to: {DEFAULT_PROMPT!r}",
    )
    parser.add_argument(
        "--list-models",
        action="store_true",
        help="Print the available models and exit.",
    )
    return parser


def select_model(models, model_name):
    if not model_name:
        available_names = ", ".join(choice["model"] for choice in models)
        raise SystemExit(
            "Please provide --model. Use --list-models to see choices.\n"
            f"Available now: {available_names}"
        )

    for choice in models:
        if choice["model"] == model_name:
            return choice

    available_names = ", ".join(choice["model"] for choice in models)
    raise SystemExit(
        f"Unknown model: {model_name}\n"
        f"Available now: {available_names}"
    )


def create_client(choice):
    from openai import OpenAI

    if choice["base_url"]:
        return OpenAI(base_url=choice["base_url"], api_key=choice["resolved_api_key"])
    return OpenAI(api_key=choice["resolved_api_key"])


def main():
    load_dotenv(override=True)
    parser = build_parser()
    args = parser.parse_args()

    print_api_key_status()

    models = build_available_models()
    if not models:
        print("\nNo models are available. Add an API key to `.env` or start Ollama locally.")
        return

    if args.list_models:
        print_models(models)
        return

    # Actual part of the code:
    selected_model = select_model(models, args.model)

    client = create_client(selected_model)
    response = client.chat.completions.create(
        model=selected_model["model"],
        messages=[{"role": "user", "content": args.prompt}],
    )

    print("\nResponse:\n")
    print(response.choices[0].message.content)

    


if __name__ == "__main__":
    main()
