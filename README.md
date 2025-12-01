# Cuizz

Lightweight CLI quizzer for the terminal. Feed it a text file with questions and four alternatives, then answer interactively.

## Build

- `gcc -o cuizz main.c`

## Run

- `./cuizz questions.txt`
- Help: `./cuizz -h`

## Question file format

```
What is the capital of France?
- Paris
- Berlin
- Madrid
- Rome
1
```

Blank lines between question blocks are allowed. The last line in each block is the correct alternative number (1-4).
