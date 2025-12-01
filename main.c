#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct question_card {
  char question[2048];
  char alternative1[2048];
  char alternative2[2048];
  char alternative3[2048];
  char alternative4[2048];
  int correct_alternative;
};

static void strip_newline(char *s) {
  size_t length = strlen(s);
  if (length > 0 && s[length - 1] == '\n') {
    s[length - 1] = '\0';
  }
}

static void copy_line_without_prefix(char *destination, size_t destination_size,
                                     const char *source, const char *prefix) {
  const char *start = source;

  size_t prefix_length = strlen(prefix);
  if (strncmp(source, prefix, prefix_length) == 0) {
    start = source + prefix_length;
  }

  strncpy(destination, start, destination_size - 1);
  destination[destination_size - 1] = '\0';
}

static int read_nonempty_line(FILE *file, char *buffer, size_t buffer_size) {
  while (fgets(buffer, buffer_size, file)) {
    if (buffer[0] == '\n' || buffer[0] == '\0') {
      continue;
    }
    strip_newline(buffer);
    return 1;
  }
  return 0;
}

static void read_alternative_line(FILE *file, char *destination,
                                  size_t destination_size, const char *label) {
  char line[2048];

  if (!fgets(line, sizeof(line), file)) {
    fprintf(stderr, "Unexpected end of file while reading %s\n", label);
    exit(EXIT_FAILURE);
  }

  strip_newline(line);

  if (strncmp(line, "- ", 2) != 0) {
    fprintf(stderr,
            "Invalid format for %s. Alternatives must start with \"- \". "
            "Got: \"%s\"\n",
            label, line);
    exit(EXIT_FAILURE);
  }

  copy_line_without_prefix(destination, destination_size, line, "- ");
}

static int read_correct_alternative(FILE *file) {
  char line[2048];
  char *endptr = NULL;

  if (!fgets(line, sizeof(line), file)) {
    fprintf(stderr,
            "Unexpected end of file while reading correct alternative\n");
    exit(EXIT_FAILURE);
  }

  strip_newline(line);

  errno = 0;
  long value = strtol(line, &endptr, 10);
  if (errno != 0 || endptr == line || value < 1 || value > 4) {
    fprintf(stderr, "Invalid correct alternative: \"%s\"\n", line);
    exit(EXIT_FAILURE);
  }

  return (int)value;
}

static void ensure_capacity(struct question_card **questions, size_t *capacity,
                            size_t used) {
  if (used < *capacity) {
    return;
  }

  *capacity *= 2;
  struct question_card *temporary =
      realloc(*questions, (*capacity) * sizeof(**questions));
  if (!temporary) {
    fprintf(stderr, "Out of memory during realloc\n");
    free(*questions);
    exit(EXIT_FAILURE);
  }

  *questions = temporary;
}

static int read_single_question(FILE *file, struct question_card *question) {
  char line[2048];
  char question_line[2048];

  if (!read_nonempty_line(file, line, sizeof(line))) {
    return 0;
  }

  strncpy(question_line, line, sizeof(question_line) - 1);
  question_line[sizeof(question_line) - 1] = '\0';

  memset(question, 0, sizeof(*question));

  strncpy(question->question, question_line, sizeof(question->question) - 1);
  question->question[sizeof(question->question) - 1] = '\0';

  read_alternative_line(file, question->alternative1,
                        sizeof(question->alternative1), "alternative 1");
  read_alternative_line(file, question->alternative2,
                        sizeof(question->alternative2), "alternative 2");
  read_alternative_line(file, question->alternative3,
                        sizeof(question->alternative3), "alternative 3");
  read_alternative_line(file, question->alternative4,
                        sizeof(question->alternative4), "alternative 4");

  question->correct_alternative = read_correct_alternative(file);
  return 1;
}

struct question_card *read_questions_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Could not open file: %s\n", filename);
    exit(EXIT_FAILURE);
  }

  size_t capacity = 256;
  size_t used = 0;

  struct question_card *questions = malloc(capacity * sizeof(*questions));
  if (!questions) {
    fprintf(stderr, "Out of memory\n");
    fclose(file);
    exit(EXIT_FAILURE);
  }

  for (;;) {
    ensure_capacity(&questions, &capacity, used);

    if (!read_single_question(file, &questions[used])) {
      break;
    }

    used++;
  }

  fclose(file);

  questions = realloc(questions, (used + 1) * sizeof(*questions));
  if (!questions) {
    fprintf(stderr, "Out of memory during final realloc\n");
    exit(EXIT_FAILURE);
  }

  memset(&questions[used], 0, sizeof(questions[used]));
  return questions;
}

void print_question_card(struct question_card question_card,
                         size_t question_number) {
  printf("\nQuestion %zu:\n", question_number + 1);
  printf("%s\n\n", question_card.question);
  printf("1. %s\n", question_card.alternative1);
  printf("2. %s\n", question_card.alternative2);
  printf("3. %s\n", question_card.alternative3);
  printf("4. %s\n", question_card.alternative4);
}

static int get_int_in_range(int minimum, int maximum) {
  for (;;) {
    char buffer[32];
    char *endptr = NULL;
    long value;

    printf("Your answer (%d-%d): ", minimum, maximum);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      printf("Input error. Please try again.\n");
      continue;
    }

    errno = 0;
    value = strtol(buffer, &endptr, 10);

    if (errno != 0 || endptr == buffer) {
      printf("Invalid input. Please enter a number between %d and %d.\n",
             minimum, maximum);
      continue;
    }

    while (*endptr == ' ' || *endptr == '\t') {
      endptr++;
    }

    if (*endptr != '\n' && *endptr != '\0') {
      printf("Unexpected characters after the number. "
             "Please enter a number between %d and %d.\n",
             minimum, maximum);
      continue;
    }

    if (value < minimum || value > maximum) {
      printf("Number out of range. Please enter a number between %d and %d.\n",
             minimum, maximum);
      continue;
    }

    return (int)value;
  }
}

int get_user_answer() { return get_int_in_range(1, 4); }

int is_correct_answer(int user_answer, struct question_card question_card) {
  if (question_card.correct_alternative == user_answer) {
    return 1;
  }
  return 0;
}

void print_result(int result, struct question_card question_card) {
  if (result) {
    printf("You got it correct!\n");
  } else {
    printf("Incorrect. The correct answer was %d\n",
           question_card.correct_alternative);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s [questions file]\n", argv[0]);
    return EXIT_FAILURE;
  }

  struct question_card *questions = read_questions_file(argv[1]);
  if (!questions) {
    printf("Failed to read questions\n");
    return EXIT_FAILURE;
  }

  printf("Cuizz: Quizzes in the terminal\n\n");

  size_t number_of_questions = 0;
  for (size_t i = 0; questions[i].correct_alternative != 0; i++) {
    number_of_questions++;
  }

  size_t correct_answers = 0;

  for (size_t index = 0; index < number_of_questions; index++) {
    struct question_card question = questions[index];

    print_question_card(question, index);
    int user_answer = get_user_answer();
    int result = is_correct_answer(user_answer, question);
    print_result(result, question);
    if (result) {
      correct_answers++;
    }
    printf("\n");
  }

  free(questions);

  printf("You got %zu/%zu questions correct!\n", correct_answers,
         number_of_questions);

  return EXIT_SUCCESS;
}
