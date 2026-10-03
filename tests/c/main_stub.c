void app_init(void); void app_tick(void);
int main(void) { app_init(); for (int i = 0; i < 10000; i++) app_tick(); return 0; }
