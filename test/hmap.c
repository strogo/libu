#include <u/libu.h>
#include <string.h>


static size_t __sample_hash (const void *key, size_t size);
static int __sample_comp(const void *key1, const void *key2);
static u_string_t *__sample_str(u_hmap_o_t *obj);
static u_hmap_o_t *__sample_obj(u_hmap_t *hmap, int key, const char *val);

static int example_easy_basic (u_test_case_t *tc)
{
      u_hmap_opts_t opts;
      u_hmap_t *hmap = NULL;
      
      u_hmap_opts_init(&opts);
      dbg_err_if (u_hmap_opts_set_val_type(&opts, U_HMAP_OPTS_DATATYPE_STRING));
          
      dbg_err_if (u_hmap_easy_new(&opts, &hmap));
      
      dbg_err_if (u_hmap_easy_put(hmap, "jack", (const void *) ":S"));
      dbg_err_if (u_hmap_easy_put(hmap, "jill", (const void *) ":)))"));
      
      u_test_case_printf(tc, "jack is %s and jill is %s",
          (const char *) u_hmap_easy_get(hmap, "jack"),
          (const char *) u_hmap_easy_get(hmap, "jill"));
      
      u_hmap_easy_free(hmap);

      return U_TEST_SUCCESS;
err:
      return U_TEST_FAILURE;
}

static int example_easy_static (u_test_case_t *tc)
{
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;

    u_dbg("example_easy_static()");

    u_hmap_opts_init(&opts);

    /* no free function needed for static data */
    u_test_err_if (u_hmap_opts_set_val_freefunc(&opts, NULL));
    u_test_err_if (u_hmap_easy_new(&opts, &hmap));

    u_test_err_if (u_hmap_easy_put(hmap, "0", "zero"));
    u_test_err_if (u_hmap_easy_put(hmap, "1", "one"));
    u_test_err_if (u_hmap_easy_put(hmap, "2", "two"));
    u_test_err_if (u_hmap_easy_put(hmap, "3", "three"));

    /* value that doesn't exist */
    u_test_err_if (u_hmap_easy_get(hmap, "4") != NULL);

    /* test deletion */
    u_test_err_if (u_hmap_easy_del(hmap, "3"));
    u_test_err_if (u_hmap_easy_get(hmap, "3") != NULL);
    u_test_err_if (u_hmap_easy_put(hmap, "3", "THREE"));

#ifdef DEBUG_HEAVY
    u_hmap_dbg(hmap);
#endif

    /* print out all values */
    u_test_err_if (strcmp(u_hmap_easy_get(hmap, "0"), "zero") != 0);
    u_test_err_if (strcmp(u_hmap_easy_get(hmap, "1"), "one") != 0);
    u_test_err_if (strcmp(u_hmap_easy_get(hmap, "2"), "two") != 0);
    u_test_err_if (strcmp(u_hmap_easy_get(hmap, "3"), "THREE") != 0);

    /* test overwrite - should fail */
    u_test_err_if (u_hmap_easy_put(hmap, "2", "TWO") == 0);
    u_test_err_if (strcmp(u_hmap_easy_get(hmap, "2"), "two") != 0);

    U_FREEF(hmap, u_hmap_easy_free);

    /* make a new hmap that _allows_ overwrite */
    u_test_err_if (u_hmap_opts_unset_option(&opts, U_HMAP_OPTS_NO_OVERWRITE));
    u_test_err_if (u_hmap_easy_new(&opts, &hmap));

    /* put elements */
    u_test_err_if (u_hmap_easy_put(hmap, "a", "alpha"));
    u_test_err_if (u_hmap_easy_put(hmap, "b", "beta"));
    u_test_err_if (u_hmap_easy_put(hmap, "a", "ALPHA"));

#ifdef DEBUG_HEAVY
    u_hmap_dbg(hmap);
#endif

    /* check elements */
    u_test_err_if (strcmp(u_hmap_easy_get(hmap, "a"), "ALPHA") != 0);
    u_test_err_if (strcmp(u_hmap_easy_get(hmap, "b"), "beta") != 0);
    
    /* free hmap (options and elements are freed automatically) */
    u_hmap_easy_free(hmap);

    return U_TEST_SUCCESS;
err:
    U_FREEF(hmap, u_hmap_easy_free);

    return U_TEST_FAILURE;
}

struct mystruct_s {
    char *a;
    char *b;
};
typedef struct mystruct_s mystruct_t;

void mystruct_free (void *val)
{
    mystruct_t *mystruct = (mystruct_t *) val;

    if (val == NULL)
        return;

    u_free(mystruct->a);
    u_free(mystruct->b);
    u_free(mystruct);
}

mystruct_t *mystruct_create (void)
{
    mystruct_t *myval = NULL;

    dbg_err_if ((myval = (mystruct_t *) u_zalloc(sizeof(mystruct_t))) == NULL);
    dbg_err_if ((myval->a = strdup("first string")) == NULL);
    dbg_err_if ((myval->b = strdup("second string")) == NULL);

    return myval;
err:
    U_FREEF(myval, mystruct_free);

    return NULL;
}

static int example_easy_dynamic (u_test_case_t *tc)
{
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    mystruct_t *mystruct;

    u_dbg("example_easy_dynamic()");

    u_hmap_opts_init(&opts);

    /* setup custom free function */
    u_test_err_if (u_hmap_opts_set_val_freefunc(&opts, &mystruct_free));
    /* no string function for custom object */
    u_test_err_if (u_hmap_opts_set_strfunc(&opts, NULL));

    u_test_err_if (u_hmap_easy_new(&opts, &hmap));

    /* insert 3 objects */
    u_test_err_if (u_hmap_easy_put(hmap, "a", mystruct_create()));
    u_test_err_if (u_hmap_easy_put(hmap, "b", mystruct_create()));
    u_test_err_if (u_hmap_easy_put(hmap, "c", mystruct_create()));

    /* test overwrite - should fail */
    u_test_err_if (u_hmap_easy_put(hmap, "b", mystruct_create()) == 0);

#ifdef DEBUG_HEAVY
    u_hmap_dbg(hmap);
#endif

    /* check a value */
    u_test_err_if ((mystruct = u_hmap_easy_get(hmap, "a")) == NULL);
    u_test_err_if (strcmp(mystruct->a, "first string") != 0);
    u_test_err_if (strcmp(mystruct->b, "second string") != 0);
    
    /* internal objects freed automatically using custom function */
    u_hmap_easy_free(hmap);

    return U_TEST_SUCCESS;
err:
    U_FREEF(hmap, u_hmap_easy_free);

    return U_TEST_FAILURE;
}

static size_t __sample_hash(const void *key, size_t size)
{
    return (*((int *) key) % size);
}

static int __sample_comp(const void *key1, const void *key2)
{
    int k1 = *((int *) key1),
        k2 = *((int *) key2);
    
    return k1 < k2 ? -1 : ((k1 > k2)? 1 : 0);
}

static u_string_t *__sample_str(u_hmap_o_t *obj)
{
    enum { MAX_OBJ_STR = 256 };
    char buf[MAX_OBJ_STR];
    u_string_t *s = NULL;

    int key = *((int *) obj->key);
    char *val = (char *) obj->val;

    dbg_err_if (u_snprintf(buf, MAX_OBJ_STR, "[%d:%s]", key, val));
    dbg_err_if (u_string_create(buf, strlen(buf)+1, &s));

    return s;

err:
    return NULL;
}



struct mystruct2_s {
    char c;
    int x;
    double d;
};
typedef struct mystruct2_s mystruct2_t;

static int example_easy_opaque (u_test_case_t *tc)
{
    enum { 
        VAL_SZ = sizeof(mystruct2_t),
        ELEMS_NUM = 4
    };
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    const char *keys[] = { "!@#", "$%^", "&*(", "()_" };
    mystruct2_t vals[] = { { 'a', 1, 1.1 }, { 'b', 2, 2.2 }, 
        { 'c', 3, 3.3 }, { 'd', 4, 4.4 } };
    mystruct2_t *pval;
    int i;

    u_dbg("example_easy_opaque()");

    u_hmap_opts_init(&opts);

    u_test_err_if (u_hmap_opts_set_val_type(&opts, U_HMAP_OPTS_DATATYPE_OPAQUE));
    u_test_err_if (u_hmap_opts_set_val_sz(&opts, VAL_SZ));

    u_test_err_if (u_hmap_easy_new(&opts, &hmap));

    /* insert elements */
    for (i = 0 ; i < ELEMS_NUM; i++) 
        u_test_err_if (u_hmap_easy_put(hmap, keys[i], &vals[i]));

#ifdef DEBUG_HEAVY
    u_hmap_dbg(hmap);
#endif

    /* check elements */
    for (i = 0; i < ELEMS_NUM; i++) {

        pval = u_hmap_easy_get(hmap, keys[i]);
        u_test_err_if (pval == NULL);

        u_test_err_if ((pval->c != vals[i].c) ||
                (pval->x != vals[i].x) ||
                (pval->d != vals[i].d));
    }

    /* free hmap (options and elements are freed automatically) */
    u_hmap_easy_free(hmap);

    return U_TEST_SUCCESS;
err:
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_FAILURE;
}

static int example_static (u_test_case_t *tc)
{
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;
    int fibonacci[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21 };

    u_dbg("example_static()");

    u_hmap_opts_init(&opts);

    /* hmap owns data by default - change it */
    u_test_err_if (u_hmap_opts_unset_option(&opts, U_HMAP_OPTS_OWNSDATA));

    u_test_err_if (u_hmap_new(&opts, &hmap));

    /* insert some sample elements */
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, "first", &fibonacci[0]), NULL));
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, "fifth", &fibonacci[4]), NULL)); 
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, "last", 
                &fibonacci[(sizeof(fibonacci)/sizeof(int))-1]), NULL));

    /* retrieve and print values to dbg */
    u_test_err_if (u_hmap_get(hmap, "last", &obj)); 
    u_dbg("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val));
    u_test_err_if (u_hmap_get(hmap, "fifth", &obj)); 
    u_dbg("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val)); 
    u_test_err_if (u_hmap_get(hmap, "first", &obj)); 
    u_dbg("hmap['%s'] = %d", (char *) obj->key, *((int *) obj->val));
    
    u_hmap_dbg(hmap);

    /* remove an element and replace it */
    u_test_err_if (u_hmap_del(hmap, "fifth", &obj)); 
    u_hmap_o_free(obj);
    
    /* check that it has been deleted */
    u_test_err_if (u_hmap_get(hmap, "fifth", &obj) == 0); 

    /* delete the other two elements */
    u_test_err_if (u_hmap_del(hmap, "last", &obj)); 
    u_hmap_o_free(obj);
    u_test_err_if (u_hmap_del(hmap, "first", &obj)); 
    u_hmap_o_free(obj);

    /* free hmap and options */
    u_hmap_free(hmap);
    
    return U_TEST_SUCCESS;
err:
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_FAILURE;
}

static int example_dynamic_own_hmap (u_test_case_t *tc)
{
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;

    u_dbg("example_dynamic_own_hmap()");

    u_hmap_opts_init(&opts);

    /* objects don't need to be freed - same as default:
    u_test_err_if (u_hmap_opts_set_freefunc(&opts, NULL));
    */

    /* hmap owns both keys and data - allow overwrite */
    u_test_err_if (u_hmap_opts_set_val_type(&opts, U_HMAP_OPTS_DATATYPE_STRING));
    u_test_err_if (u_hmap_opts_unset_option(&opts, U_HMAP_OPTS_NO_OVERWRITE));
    u_test_err_if (u_hmap_new(&opts, &hmap));

    /* insert some sample elements */
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, "EN", "Hello world!"), NULL));
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, "IT", "Ciao mondo!"), NULL));
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, "DE", "Hallo Welt!"), NULL));

    /* retrieve and print values to console */
    u_test_err_if (u_hmap_get(hmap, "DE", &obj)); 
    u_test_err_if (u_hmap_get(hmap, "EN", &obj)); 

    /* remove an element and replace it */
    u_test_err_if (u_hmap_del(hmap, "DE", NULL)); 
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, "DE", "Auf Wiedersehen!"), NULL));
    u_test_err_if (u_hmap_get(hmap, "DE", &obj)); 

    /* check some values */
    u_test_err_if (u_hmap_get(hmap, "IT", &obj)); 
    u_test_err_if (strcmp(obj->val, "Ciao mondo!") != 0);
    u_test_err_if (u_hmap_get(hmap, "DE", &obj)); 
    u_test_err_if (strcmp(obj->val, "Auf Wiedersehen!") != 0);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_free(hmap);

    return U_TEST_SUCCESS;

err:
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_FAILURE;
}

static int example_dynamic_own_user (u_test_case_t *tc)
{
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL;

#define OBJ_FREE(obj) \
    if (obj) { \
        u_free(obj->key); \
        u_free(obj->val); \
        u_hmap_o_free(obj); \
        obj = NULL; \
    }
    
    u_dbg("example_dynamic_own_user()");

    u_hmap_opts_init(&opts);

    /* hmap owns data by default - change it */
    u_test_err_if (u_hmap_opts_unset_option(&opts, U_HMAP_OPTS_OWNSDATA));

    /* no overwrites allowed by default - change this too  */
    u_test_err_if (u_hmap_opts_unset_option(&opts, U_HMAP_OPTS_NO_OVERWRITE));

    u_test_err_if (u_hmap_new(&opts, &hmap));

    /* insert some sample elements */
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, (void *) u_strdup("EN"), 
                    (void *) u_strdup("Hello world!")), &obj));
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, (void *) u_strdup("IT"), 
                    (void *) u_strdup("Ciao mondo!")), &obj));
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, (void *) u_strdup("DE"), 
                    (void *) u_strdup("Hallo Welt!")), &obj));

    /* retrieve and print values to console */
    u_test_err_if (u_hmap_get(hmap, "IT", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    u_test_err_if (u_hmap_get(hmap, "DE", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);
    u_test_err_if (u_hmap_get(hmap, "EN", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    /* remove an element and replace it */
    u_test_err_if (u_hmap_del(hmap, "DE", &obj)); 
    OBJ_FREE(obj);

    /* check that it has been deleted */
    u_test_err_if (u_hmap_get(hmap, "DE", &obj) == 0); 

    /* replace with a new element and print it */
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, (void *) u_strdup("DE"), 
                    (void *) u_strdup("Auf Wiedersehen!")), NULL));
    u_test_err_if (u_hmap_put(hmap, u_hmap_o_new(hmap, (void *) u_strdup("DE"), 
                    (void *) u_strdup("Auf Wiedersehen2!")), &obj));
    OBJ_FREE(obj);
    u_test_err_if (u_hmap_get(hmap, "DE", &obj)); 
    u_dbg("hmap['%s'] = %s", (char *) obj->key, (char *) obj->val);

    u_hmap_del(hmap, "IT", &obj);
    OBJ_FREE(obj);
    u_hmap_del(hmap, "DE", &obj);
    OBJ_FREE(obj);
    u_hmap_del(hmap, "EN", &obj);
    OBJ_FREE(obj);

    /* free hmap (options and elements are freed automatically) */
    u_hmap_free(hmap);

    return U_TEST_SUCCESS;

err:
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_FAILURE;
}

/* Allocate (key, value) pair dynamically */
static u_hmap_o_t *__sample_obj(u_hmap_t *hmap, int key, const char *val)
{
    u_hmap_o_t *new = NULL;

    int *k = NULL;
    char *v = NULL;
    
    k = (int *) malloc(sizeof(int));
    dbg_err_if (k == NULL);
    *k = key;

    v = u_strdup(val);
    dbg_err_if (v == NULL);
    
    new = u_hmap_o_new(hmap, k, v);
    dbg_err_if (new == NULL);
    
    return new;

err:
    u_free(k);
    u_free(v);
    
    return NULL;
}

static int example_types_custom (u_test_case_t *tc)
{
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    u_hmap_o_t *obj = NULL; 
    int keys[] = { 2, 1, 4, 7, 4, 3, 6, 1, 5 };
    const char *vals[] = { "two", "one", "four", "seven", "four2", "three", 
        "six", "one2", "five" };
    int i, x;
    
    u_dbg("example_types_custom()"); 

    u_hmap_opts_init(&opts);

    u_test_err_if (u_hmap_opts_set_option(&opts, U_HMAP_OPTS_HASH_STRONG));
    u_test_err_if (u_hmap_opts_unset_option(&opts, U_HMAP_OPTS_NO_OVERWRITE));
    
    u_test_err_if (u_hmap_opts_set_key_type(&opts, U_HMAP_OPTS_DATATYPE_OPAQUE));
    u_test_err_if (u_hmap_opts_set_key_sz(&opts, sizeof(int)));

    u_test_err_if (u_hmap_opts_set_val_type(&opts, U_HMAP_OPTS_DATATYPE_STRING));

    u_test_err_if (u_hmap_opts_set_size(&opts, 3));
    u_test_err_if (u_hmap_opts_set_hashfunc(&opts, &__sample_hash));
    u_test_err_if (u_hmap_opts_set_compfunc(&opts, &__sample_comp));
    u_test_err_if (u_hmap_opts_set_strfunc(&opts, &__sample_str));

    u_test_err_if (u_hmap_new(&opts, &hmap));

    for (i = 0; i < 9; i++) {
        u_test_err_if ((obj = u_hmap_o_new(hmap, &keys[i], vals[i])) == NULL);
        u_test_err_if (u_hmap_put(hmap, obj, NULL));
    }

#ifdef DEBUG_HEAVY
    u_hmap_dbg(hmap);
#endif

    for (i = 0; i < 9; i++) {
        u_test_err_if (u_hmap_get(hmap, &keys[i], &obj)); 
        u_dbg("o: %s, v: %s", obj->val, vals[i]);
    }

    u_hmap_free(hmap);

    return U_TEST_SUCCESS;
err:
    U_FREEF(hmap, u_hmap_free);

    return U_TEST_FAILURE;
}

static int test_resize (u_test_case_t *tc)
{
    enum { NUM_ELEMS = 100000, MAX_STR = 256 };
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    int i = 0;
    char key[MAX_STR], 
         val[MAX_STR];

    u_dbg("test_resize()");

    u_hmap_opts_init(&opts);

    u_test_err_if (u_hmap_opts_set_val_type(&opts, U_HMAP_OPTS_DATATYPE_STRING));
    u_test_err_if (u_hmap_opts_set_size(&opts, 3));

    u_test_err_if (u_hmap_easy_new(&opts, &hmap));

    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        u_snprintf(val, MAX_STR, "val%d", i);
        u_test_err_if (u_hmap_easy_put(hmap, key, val));
    }

    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        u_test_err_if (u_hmap_easy_del(hmap, key)); 
    }

    u_hmap_easy_free(hmap);
    
    return U_TEST_SUCCESS;

err:
    U_FREEF(hmap, u_hmap_easy_free);

    return U_TEST_FAILURE;
}

static int test_linear (u_test_case_t *tc)
{
    enum { NUM_ELEMS = 100000, MAX_STR = 256 };
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    int i = 0;
    char key[MAX_STR], 
         val[MAX_STR];

    u_dbg("test_linear()");

    u_hmap_opts_init(&opts);

    u_test_err_if (u_hmap_opts_set_val_type(&opts, U_HMAP_OPTS_DATATYPE_STRING));
    u_test_err_if (u_hmap_opts_set_size(&opts, 1000));
    u_test_err_if (u_hmap_opts_set_type(&opts, U_HMAP_TYPE_LINEAR));

    u_test_err_if (u_hmap_easy_new(&opts, &hmap));

    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        u_snprintf(val, MAX_STR, "val%d", i);
        u_test_err_if (u_hmap_easy_put(hmap, key, val));
    }

    for (i = 0; i < NUM_ELEMS; ++i) {
        u_snprintf(key, MAX_STR, "key%d", i);
        u_test_err_if (u_hmap_easy_del(hmap, key)); 
    }

    u_hmap_easy_free(hmap);
    
    return U_TEST_SUCCESS;

err:
    U_FREEF(hmap, u_hmap_easy_free);

    return U_TEST_FAILURE;
}

/** 
 * keys have limited scope
 * values have wide scope 
 */
static int test_scope (u_test_case_t *tc)
{
    enum { KEY_SZ = 256 };
    int i;
    u_hmap_opts_t opts;
    u_hmap_t *hmap = NULL;
    char *vals[] = { "zero", "one", "two", "three", "four", "five",
        "six", "seven", "eight", "nine" };

    u_dbg("test_scope()");

    u_hmap_opts_init(&opts);

    /* static data - no free function required */
    u_test_err_if (u_hmap_opts_set_val_freefunc(&opts, NULL));

    /* default is string key and pointer value */
    u_test_err_if (u_hmap_easy_new(&opts, &hmap));

    {
        char key[KEY_SZ];
        for (i = 0; i < 10; i++) {
            u_snprintf(key, KEY_SZ, "key%d", i);
            u_test_err_if (u_hmap_easy_put(hmap, key, vals[i]));
        }
    }

#ifdef DEBUG_HEAVY
    u_hmap_dbg(hmap);
#endif

    {
        char key[KEY_SZ];
        char key2[KEY_SZ];
        for (i = 0; i < 10; i++) {
            u_snprintf(key2, KEY_SZ, "key%d", i);
            u_test_err_if (strcmp(u_hmap_easy_get(hmap, key2), vals[i]) != 0);
        }
    }

    u_hmap_easy_free(hmap);

    return U_TEST_SUCCESS;
err:
    if (hmap)
        u_hmap_easy_free(hmap);

    return U_TEST_FAILURE;
}

int test_suite_hmap_register (u_test_t *t)
{
    u_test_suite_t *ts = NULL;

    con_err_if (u_test_suite_new("Hash Map", &ts));

    /* examples */
    con_err_if (u_test_case_register("Static Example", 
                example_static, ts));
    con_err_if (u_test_case_register("Basic (hmap_easy interface)",
                example_easy_basic, ts));
    con_err_if (u_test_case_register("Static (hmap_easy interface)", 
                example_easy_static, ts));
    con_err_if (u_test_case_register("Dynamic (hmap_easy interface)",
                example_easy_dynamic, ts));
    con_err_if (u_test_case_register("Opaque (hmap_easy interface)", 
                example_easy_opaque, ts));
    con_err_if (u_test_case_register("Dynamic, Hmap Owns",
                example_dynamic_own_hmap, ts));
    con_err_if (u_test_case_register("Dynamic, User Owns",
                example_dynamic_own_user, ts));
    con_err_if (u_test_case_register("Custom Handlers",
                example_types_custom, ts));

    /* tests */
    con_err_if (u_test_case_register("Resize", test_resize, ts));
    con_err_if (u_test_case_register("Linear Probing", test_linear, ts));
    con_err_if (u_test_case_register("Scoping", test_scope, ts));

    /* hmap depends on the strings module */
    con_err_if (u_test_suite_dep_register("Strings", ts));

    return u_test_suite_add(ts, t);
err:
    u_test_suite_free(ts);
    return ~0;
}
