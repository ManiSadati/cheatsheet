import argparse
import os
from dotenv import load_dotenv
from bs4 import BeautifulSoup
import requests
from IPython.display import Markdown, display


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
        "provider": "Ollama",
        "label": "Gemma 3 270m",
        "model": "gemma3:270m",
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




# Standard headers to fetch a website
headers = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/117.0.0.0 Safari/537.36"
}


def fetch_website_contents(url):
    """
    Return the title and contents of the website at the given url;
    truncate to 2,000 characters as a sensible limit
    """
    response = requests.get(url, headers=headers)
    soup = BeautifulSoup(response.content, "html.parser")
    title = soup.title.string if soup.title else "No title found"
    if soup.body:
        for irrelevant in soup.body(["script", "style", "img", "input"]):
            irrelevant.decompose()
        text = soup.body.get_text(separator="\n", strip=True)
    else:
        text = ""
    return (title + "\n\n" + text)[:2_000]


def fetch_website_links(url):
    """
    Return the links on the webiste at the given url
    I realize this is inefficient as we're parsing twice! This is to keep the code in the lab simple.
    Feel free to use a class and optimize it!
    """
    response = requests.get(url, headers=headers)
    soup = BeautifulSoup(response.content, "html.parser")
    links = [link.get("href") for link in soup.find_all("a")]
    return [link for link in links if link]

def messages_for(website):

    system_prompt = """
    You are a assistant that analyzes the contents of a website,
    and provides a short, humorous summary, ignoring text that might be navigation related.
    Respond in markdown. Do not wrap the markdown in a code block - respond just with the markdown.
    """


    user_prompt_prefix = """
    Here are the contents of a website.
    Provide a short summary of this website.
    If it includes news or announcements, then summarize these too.
    """

    return [
        {"role": "system", "content": system_prompt},
        {"role": "user", "content": user_prompt_prefix + website}
    ]

def summarize(url, selected_model):
    website = fetch_website_contents(url)

    client = create_client(selected_model)
    response = client.chat.completions.create(
        model=selected_model["model"],
        messages=messages_for(website),
    )
    print(response.choices[0].message.content)
    return response.choices[0].message.content

def display_summary(url, selected_model):
    summary = summarize(url, selected_model)
    display(Markdown(summary))

def print_stream(response):
    chat_bot_answer = "chatbot: "
    for chunk in response:
        chunk_answer = chunk.choices[0].delta.content or ''
        print( chunk_answer, end="", flush=True)
        chat_bot_answer +=  chunk_answer
    print("")
    return chat_bot_answer


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

    system_prompt = "You are a AI chatbot assistant, help the user! here is the history of the chat: "
    user_prompt = ""
    print("Hi, how can I help you?")
    chat_history = "chatbot: Hi, how can I help you?"
    while(True):
        print(">> ", end = "")
        user_prompt = input()
        print("(thinking...)")
        
        messages = [{"role": "system", "content": system_prompt + chat_history}, 
                    {"role": "user", "content": user_prompt}]
        response = client.chat.completions.create(
            model=selected_model["model"],
            messages=messages,
            stream=True
            )
        
        chat_history += "user: " + user_prompt
        
        chat_bot_answer = print_stream(response)
        chat_history += chat_bot_answer


if __name__ == "__main__":
    main()
