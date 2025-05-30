#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define MAX_WAITING_CHAIRS 5
#define NUM_STUDENTS 10

// سمافورها و متغیرهای هم‌زمانی
sem_t students_sem;    // دانشجویان منتظر
sem_t ta_sem;          // دستیار آموزشی
pthread_mutex_t mutex; // قفل برای متغیرهای مشترک

int waiting_students = 0; // تعداد دانشجویان منتظر
int ta_sleeping = 1;      // وضعیت خواب/بیداری TA

// تابع دستیار آموزشی
void *ta_thread(void *arg)
{
    while (1)
    {
        // منتظر دانشجو می‌ماند
        sem_wait(&students_sem);

        pthread_mutex_lock(&mutex);
        ta_sleeping = 0;    // TA بیدار است
        waiting_students--; // یک دانشجو از صف خارج می‌شود
        pthread_mutex_unlock(&mutex);

        printf("TA is helping a student. Students waiting: %d\n", waiting_students);
        sleep(rand() % 3 + 1); // زمان کمک به دانشجو

        pthread_mutex_lock(&mutex);
        if (waiting_students == 0)
        {
            ta_sleeping = 1; // اگر دانشجویی نبود، TA می‌خوابد
            printf("TA is sleeping.\n");
        }
        pthread_mutex_unlock(&mutex);

        // به TA سیگنال می‌دهیم که کارش تمام شده
        sem_post(&ta_sem);
    }
    return NULL;
}

// تابع دانشجو
void *student_thread(void *arg)
{
    int student_id = *(int *)arg;

    while (1)
    {
        // دانشجو برای مدتی مطالعه می‌کند
        sleep(rand() % 5 + 1);

        pthread_mutex_lock(&mutex);

        if (waiting_students < MAX_WAITING_CHAIRS)
        {
            waiting_students++;
            printf("Student %d arrived. Students waiting: %d\n", student_id, waiting_students);

            if (ta_sleeping)
            {
                printf("Student %d woke up the TA.\n", student_id);
            }

            // به دانشجویان سیگنال می‌دهیم
            sem_post(&students_sem);
            pthread_mutex_unlock(&mutex);

            // منتظر TA می‌ماند
            sem_wait(&ta_sem);

            printf("Student %d got help from TA.\n", student_id);
            break; // بعد از دریافت کمک، دانشجو می‌رود
        }
        else
        {
            // اگر صندلی خالی نبود
            printf("Student %d found no empty chair and will return later.\n", student_id);
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

int main()
{
    pthread_t ta;
    pthread_t students[NUM_STUDENTS];
    int student_ids[NUM_STUDENTS];

    // مقداردهی اولیه سمافورها و mutex
    sem_init(&students_sem, 0, 0);
    sem_init(&ta_sem, 0, 1);
    pthread_mutex_init(&mutex, NULL);

    srand(time(NULL));

    // ایجاد نخ TA
    pthread_create(&ta, NULL, ta_thread, NULL);

    // ایجاد نخ‌های دانشجویان
    for (int i = 0; i < NUM_STUDENTS; i++)
    {
        student_ids[i] = i + 1;
        pthread_create(&students[i], NULL, student_thread, &student_ids[i]);
    }

    // منتظر ماندن برای پایان دانشجویان
    for (int i = 0; i < NUM_STUDENTS; i++)
    {
        pthread_join(students[i], NULL);
    }

    // پایان دادن به نخ TA (در واقعیت این بخش اجرا نمی‌شود چون TA در حلقه بی‌نهایت است)
    pthread_cancel(ta);

    // آزاد کردن منابع
    sem_destroy(&students_sem);
    sem_destroy(&ta_sem);
    pthread_mutex_destroy(&mutex);

    return 0;
}