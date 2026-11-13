#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define NAME_LEN 50
#define INITIAL_CAPACITY 10
#define LOW_STOCK_LIMIT 10
#define NEAR_EXPIRY_DAYS 30
#define DATA_FILE "pharmacy_data.txt"

typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef struct {
    int id;
    char name[NAME_LEN];
    Date expiry;
    float price;
    int stock;
} Drug;

typedef struct {
    Drug *items;
    int count;
    int capacity;
} Inventory;

void readLine(const char *prompt, char *buf, int size) {
    size_t len;
    int c;
    printf("%s", prompt);
    if (fgets(buf, size, stdin) == NULL) {
        printf("\nInput closed. Exiting.\n");
        exit(0);
    }
    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
}

int readInt(const char *prompt, int min, int max) {
    char buf[64];
    char *end;
    long v;
    while (1) {
        readLine(prompt, buf, sizeof(buf));
        v = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && v >= min && v <= max) {
            return (int)v;
        }
        printf("Invalid input. Enter a whole number between %d and %d.\n", min, max);
    }
}

double readDouble(const char *prompt, double min, double max) {
    char buf[64];
    char *end;
    double v;
    while (1) {
        readLine(prompt, buf, sizeof(buf));
        v = strtod(buf, &end);
        if (end != buf && *end == '\0' && v >= min && v <= max) {
            return v;
        }
        printf("Invalid input. Enter a number between %.2f and %.2f.\n", min, max);
    }
}

void readName(const char *prompt, char *buf, int size) {
    while (1) {
        readLine(prompt, buf, size);
        if (strlen(buf) == 0) {
            printf("Name cannot be empty.\n");
        } else if (strchr(buf, '|') != NULL) {
            printf("Name cannot contain the '|' character.\n");
        } else {
            return;
        }
    }
}

int confirm(const char *prompt) {
    char buf[16];
    readLine(prompt, buf, sizeof(buf));
    return buf[0] == 'y' || buf[0] == 'Y';
}

int compareNoCase(const char *a, const char *b) {
    int ca;
    int cb;
    while (*a && *b) {
        ca = tolower((unsigned char)*a);
        cb = tolower((unsigned char)*b);
        if (ca != cb) {
            return ca - cb;
        }
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int containsNoCase(const char *hay, const char *needle) {
    size_t n = strlen(needle);
    size_t h = strlen(hay);
    size_t i;
    size_t j;
    if (n == 0) {
        return 1;
    }
    if (n > h) {
        return 0;
    }
    for (i = 0; i + n <= h; i++) {
        for (j = 0; j < n; j++) {
            if (tolower((unsigned char)hay[i + j]) != tolower((unsigned char)needle[j])) {
                break;
            }
        }
        if (j == n) {
            return 1;
        }
    }
    return 0;
}

Date getToday(void) {
    time_t now = time(NULL);
    struct tm *info = localtime(&now);
    Date d;
    d.day = info->tm_mday;
    d.month = info->tm_mon + 1;
    d.year = info->tm_year + 1900;
    return d;
}

int isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

int isValidDate(Date d) {
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay;
    if (d.year < 1900 || d.year > 2200 || d.month < 1 || d.month > 12) {
        return 0;
    }
    maxDay = daysInMonth[d.month - 1];
    if (d.month == 2 && isLeapYear(d.year)) {
        maxDay = 29;
    }
    return d.day >= 1 && d.day <= maxDay;
}

int compareDates(Date a, Date b) {
    if (a.year != b.year) {
        return a.year - b.year;
    }
    if (a.month != b.month) {
        return a.month - b.month;
    }
    return a.day - b.day;
}

long toDayNumber(Date d) {
    int y = d.year;
    int m = d.month;
    long era;
    long yoe;
    long doy;
    long doe;
    if (m <= 2) {
        y--;
    }
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d.day - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe;
}

long daysUntil(Date target, Date today) {
    return toDayNumber(target) - toDayNumber(today);
}

Date readDate(const char *label) {
    Date d;
    printf("%s\n", label);
    while (1) {
        d.day = readInt("  Day   : ", 1, 31);
        d.month = readInt("  Month : ", 1, 12);
        d.year = readInt("  Year  : ", 1900, 2200);
        if (isValidDate(d)) {
            return d;
        }
        printf("That date does not exist. Please enter it again.\n");
    }
}

int isExpired(const Drug *d, Date today) {
    return compareDates(d->expiry, today) < 0;
}

const char *statusText(const Drug *d, Date today) {
    if (isExpired(d, today)) {
        return "EXPIRED";
    }
    if (daysUntil(d->expiry, today) <= NEAR_EXPIRY_DAYS) {
        return "NEAR EXPIRY";
    }
    if (d->stock < LOW_STOCK_LIMIT) {
        return "LOW STOCK";
    }
    return "OK";
}

void initInventory(Inventory *inv) {
    inv->items = (Drug *)malloc(INITIAL_CAPACITY * sizeof(Drug));
    if (inv->items == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    inv->count = 0;
    inv->capacity = INITIAL_CAPACITY;
}

void freeInventory(Inventory *inv) {
    free(inv->items);
    inv->items = NULL;
    inv->count = 0;
    inv->capacity = 0;
}

int addToInventory(Inventory *inv, const Drug *d) {
    Drug *tmp;
    int newCap;
    if (inv->count == inv->capacity) {
        newCap = inv->capacity * 2;
        tmp = (Drug *)realloc(inv->items, (size_t)newCap * sizeof(Drug));
        if (tmp == NULL) {
            return 0;
        }
        inv->items = tmp;
        inv->capacity = newCap;
    }
    inv->items[inv->count] = *d;
    inv->count++;
    return 1;
}

Drug *findById(Inventory *inv, int id) {
    int i;
    for (i = 0; i < inv->count; i++) {
        if (inv->items[i].id == id) {
            return &inv->items[i];
        }
    }
    return NULL;
}

int adjustStock(Drug *d, int delta) {
    if (d->stock + delta < 0) {
        return 0;
    }
    d->stock += delta;
    return 1;
}

void removeAt(Inventory *inv, int index) {
    int i;
    for (i = index; i < inv->count - 1; i++) {
        inv->items[i] = inv->items[i + 1];
    }
    inv->count--;
}

int saveInventory(const Inventory *inv) {
    FILE *fp = fopen(DATA_FILE, "w");
    int i;
    const Drug *d;
    if (fp == NULL) {
        return 0;
    }
    for (i = 0; i < inv->count; i++) {
        d = &inv->items[i];
        fprintf(fp, "%d|%s|%d|%d|%d|%.2f|%d\n", d->id, d->name,
                d->expiry.day, d->expiry.month, d->expiry.year, d->price, d->stock);
    }
    fclose(fp);
    return 1;
}

void loadInventory(Inventory *inv) {
    FILE *fp = fopen(DATA_FILE, "r");
    char line[256];
    Drug d;
    int loaded = 0;
    int skipped = 0;
    if (fp == NULL) {
        printf("No saved data found. Starting with an empty inventory.\n");
        return;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        if (sscanf(line, "%d|%49[^|]|%d|%d|%d|%f|%d", &d.id, d.name,
                   &d.expiry.day, &d.expiry.month, &d.expiry.year, &d.price, &d.stock) == 7
            && d.id > 0 && isValidDate(d.expiry) && d.price >= 0 && d.stock >= 0
            && findById(inv, d.id) == NULL && addToInventory(inv, &d)) {
            loaded++;
        } else {
            skipped++;
        }
    }
    fclose(fp);
    printf("Loaded %d drug(s) from file.", loaded);
    if (skipped > 0) {
        printf(" Skipped %d invalid record(s).", skipped);
    }
    printf("\n");
}

void printHeader(void) {
    printf("\n%-6s %-24s %-10s %10s %7s  %-12s\n", "ID", "Name", "Expiry", "Price", "Stock", "Status");
    printf("---------------------------------------------------------------------------\n");
}

void printRow(const Drug *d, Date today) {
    printf("%-6d %-24.24s %02d/%02d/%04d %10.2f %7d  %-12s\n", d->id, d->name,
           d->expiry.day, d->expiry.month, d->expiry.year, d->price, d->stock,
           statusText(d, today));
}

void addDrug(Inventory *inv) {
    Drug d;
    Date today = getToday();
    d.id = readInt("Enter Drug ID (positive number): ", 1, 999999);
    if (findById(inv, d.id) != NULL) {
        printf("A drug with ID %d already exists.\n", d.id);
        return;
    }
    readName("Enter Drug Name: ", d.name, NAME_LEN);
    d.expiry = readDate("Enter Expiry Date:");
    d.price = (float)readDouble("Enter Price per unit: ", 0.0, 1000000.0);
    d.stock = readInt("Enter Stock Quantity: ", 0, 1000000);
    if (!addToInventory(inv, &d)) {
        printf("Memory allocation failed. Drug not added.\n");
        return;
    }
    printf("Drug added successfully.\n");
    if (isExpired(&d, today)) {
        printf("ALERT: This drug is already expired!\n");
    }
}

void displayAll(Inventory *inv) {
    int i;
    Date today = getToday();
    if (inv->count == 0) {
        printf("No drugs in inventory.\n");
        return;
    }
    printHeader();
    for (i = 0; i < inv->count; i++) {
        printRow(&inv->items[i], today);
    }
    printf("\nTotal drug records: %d\n", inv->count);
}

void searchById(Inventory *inv) {
    Drug *d;
    Date today = getToday();
    int id = readInt("Enter Drug ID to search: ", 1, 999999);
    d = findById(inv, id);
    if (d == NULL) {
        printf("Drug not found.\n");
        return;
    }
    printHeader();
    printRow(d, today);
}

void searchByName(Inventory *inv) {
    char key[NAME_LEN];
    int i;
    int found = 0;
    Date today = getToday();
    readName("Enter name or part of name: ", key, NAME_LEN);
    for (i = 0; i < inv->count; i++) {
        if (containsNoCase(inv->items[i].name, key)) {
            if (!found) {
                printHeader();
            }
            printRow(&inv->items[i], today);
            found++;
        }
    }
    if (!found) {
        printf("No matching drugs found.\n");
    } else {
        printf("\n%d match(es) found.\n", found);
    }
}

void restockDrug(Inventory *inv) {
    Drug *d;
    int qty;
    Date today = getToday();
    int id = readInt("Enter Drug ID to restock: ", 1, 999999);
    d = findById(inv, id);
    if (d == NULL) {
        printf("Drug not found.\n");
        return;
    }
    printf("Current: %s | Stock: %d | Status: %s\n", d->name, d->stock, statusText(d, today));
    qty = readInt("Enter quantity to add: ", 1, 1000000);
    if (isExpired(d, today)) {
        printf("The existing batch is expired and will be discarded.\n");
        d->expiry = readDate("Enter expiry date of the new batch:");
        if (compareDates(d->expiry, today) < 0) {
            printf("The new batch is also expired. Restock cancelled.\n");
            return;
        }
        d->stock = 0;
    }
    adjustStock(d, qty);
    printf("Restocked. New stock of %s: %d\n", d->name, d->stock);
}

void sellDrug(Inventory *inv) {
    Drug *d;
    int qty;
    Date today = getToday();
    int id = readInt("Enter Drug ID to dispense: ", 1, 999999);
    d = findById(inv, id);
    if (d == NULL) {
        printf("Drug not found.\n");
        return;
    }
    if (isExpired(d, today)) {
        printf("ALERT: %s is expired and cannot be dispensed.\n", d->name);
        return;
    }
    printf("Available stock of %s: %d\n", d->name, d->stock);
    qty = readInt("Enter quantity to dispense: ", 1, 1000000);
    if (!adjustStock(d, -qty)) {
        printf("Insufficient stock. Available: %d\n", d->stock);
        return;
    }
    printf("Dispensed %d unit(s). Bill amount: %.2f. Remaining stock: %d\n", qty, qty * d->price, d->stock);
    if (d->stock < LOW_STOCK_LIMIT) {
        printf("WARNING: Low stock for %s. Please restock soon.\n", d->name);
    }
}

void updatePrice(Inventory *inv) {
    Drug *d;
    int id = readInt("Enter Drug ID: ", 1, 999999);
    d = findById(inv, id);
    if (d == NULL) {
        printf("Drug not found.\n");
        return;
    }
    printf("Current price of %s: %.2f\n", d->name, d->price);
    d->price = (float)readDouble("Enter new price: ", 0.0, 1000000.0);
    printf("Price updated to %.2f.\n", d->price);
}

void expiryAlerts(Inventory *inv) {
    int i;
    int found = 0;
    long days;
    Date today = getToday();
    printf("\n--- Expired Products ---\n");
    for (i = 0; i < inv->count; i++) {
        if (isExpired(&inv->items[i], today)) {
            printf("ALERT: [%d] %s expired on %02d/%02d/%04d (Stock: %d)\n",
                   inv->items[i].id, inv->items[i].name,
                   inv->items[i].expiry.day, inv->items[i].expiry.month,
                   inv->items[i].expiry.year, inv->items[i].stock);
            found++;
        }
    }
    if (!found) {
        printf("No expired products.\n");
    }
    found = 0;
    printf("\n--- Expiring Within %d Days ---\n", NEAR_EXPIRY_DAYS);
    for (i = 0; i < inv->count; i++) {
        if (!isExpired(&inv->items[i], today)) {
            days = daysUntil(inv->items[i].expiry, today);
            if (days <= NEAR_EXPIRY_DAYS) {
                printf("WARNING: [%d] %s expires in %ld day(s)\n",
                       inv->items[i].id, inv->items[i].name, days);
                found++;
            }
        }
    }
    if (!found) {
        printf("No products nearing expiry.\n");
    }
}

void lowStockReport(Inventory *inv) {
    int i;
    int found = 0;
    Date today = getToday();
    for (i = 0; i < inv->count; i++) {
        if (inv->items[i].stock < LOW_STOCK_LIMIT) {
            if (!found) {
                printf("\n--- Drugs Below %d Units ---", LOW_STOCK_LIMIT);
                printHeader();
            }
            printRow(&inv->items[i], today);
            found++;
        }
    }
    if (!found) {
        printf("No drugs are low on stock.\n");
    }
}

int cmpName(const void *a, const void *b) {
    return compareNoCase(((const Drug *)a)->name, ((const Drug *)b)->name);
}

int cmpExpiry(const void *a, const void *b) {
    return compareDates(((const Drug *)a)->expiry, ((const Drug *)b)->expiry);
}

int cmpStock(const void *a, const void *b) {
    return ((const Drug *)a)->stock - ((const Drug *)b)->stock;
}

int cmpPrice(const void *a, const void *b) {
    float pa = ((const Drug *)a)->price;
    float pb = ((const Drug *)b)->price;
    return (pa > pb) - (pa < pb);
}

void sortInventory(Inventory *inv) {
    int choice;
    if (inv->count < 2) {
        printf("Not enough drugs to sort.\n");
        return;
    }
    printf("\nSort by:\n1. Name\n2. Expiry Date\n3. Stock (low to high)\n4. Price (low to high)\n");
    choice = readInt("Enter choice: ", 1, 4);
    switch (choice) {
        case 1:
            qsort(inv->items, (size_t)inv->count, sizeof(Drug), cmpName);
            break;
        case 2:
            qsort(inv->items, (size_t)inv->count, sizeof(Drug), cmpExpiry);
            break;
        case 3:
            qsort(inv->items, (size_t)inv->count, sizeof(Drug), cmpStock);
            break;
        default:
            qsort(inv->items, (size_t)inv->count, sizeof(Drug), cmpPrice);
            break;
    }
    printf("Inventory sorted.\n");
    displayAll(inv);
}

void inventorySummary(Inventory *inv) {
    int i;
    int units = 0;
    int expiredCount = 0;
    int lowCount = 0;
    double validValue = 0.0;
    double expiredValue = 0.0;
    Date today = getToday();
    for (i = 0; i < inv->count; i++) {
        units += inv->items[i].stock;
        if (isExpired(&inv->items[i], today)) {
            expiredCount++;
            expiredValue += inv->items[i].price * inv->items[i].stock;
        } else {
            validValue += inv->items[i].price * inv->items[i].stock;
        }
        if (inv->items[i].stock < LOW_STOCK_LIMIT) {
            lowCount++;
        }
    }
    printf("\n===== INVENTORY SUMMARY =====\n");
    printf("Total drug records      : %d\n", inv->count);
    printf("Total units in stock    : %d\n", units);
    printf("Expired drugs           : %d\n", expiredCount);
    printf("Low stock drugs         : %d\n", lowCount);
    printf("Value of usable stock   : %.2f\n", validValue);
    printf("Value of expired stock  : %.2f\n", expiredValue);
}

void removeDrug(Inventory *inv) {
    Drug *d;
    int id = readInt("Enter Drug ID to remove: ", 1, 999999);
    d = findById(inv, id);
    if (d == NULL) {
        printf("Drug not found.\n");
        return;
    }
    printf("Selected: %s (Stock: %d)\n", d->name, d->stock);
    if (confirm("Are you sure you want to remove this drug? (y/n): ")) {
        removeAt(inv, (int)(d - inv->items));
        printf("Drug removed.\n");
    } else {
        printf("Removal cancelled.\n");
    }
}

void removeExpired(Inventory *inv) {
    int i = 0;
    int removed = 0;
    int expiredCount = 0;
    Date today = getToday();
    for (i = 0; i < inv->count; i++) {
        if (isExpired(&inv->items[i], today)) {
            expiredCount++;
        }
    }
    if (expiredCount == 0) {
        printf("No expired drugs to remove.\n");
        return;
    }
    printf("%d expired drug(s) found.\n", expiredCount);
    if (!confirm("Remove all expired drugs? (y/n): ")) {
        printf("Cancelled.\n");
        return;
    }
    i = 0;
    while (i < inv->count) {
        if (isExpired(&inv->items[i], today)) {
            removeAt(inv, i);
            removed++;
        } else {
            i++;
        }
    }
    printf("Removed %d expired drug(s).\n", removed);
}

void saveAndReport(const Inventory *inv) {
    if (saveInventory(inv)) {
        printf("Data saved to %s.\n", DATA_FILE);
    } else {
        printf("Error: could not write to %s.\n", DATA_FILE);
    }
}

void showMenu(void) {
    printf("\n========== HOSPITAL PHARMACY STOCK SYSTEM ==========\n");
    printf(" 1. Add New Drug\n");
    printf(" 2. Display All Drugs\n");
    printf(" 3. Search Drug by ID\n");
    printf(" 4. Search Drug by Name\n");
    printf(" 5. Restock Drug\n");
    printf(" 6. Dispense Drug\n");
    printf(" 7. Update Price\n");
    printf(" 8. Expiry Alerts\n");
    printf(" 9. Low Stock Report\n");
    printf("10. Sort Inventory\n");
    printf("11. Inventory Summary\n");
    printf("12. Remove a Drug\n");
    printf("13. Remove All Expired Drugs\n");
    printf("14. Save Data\n");
    printf(" 0. Save and Exit\n");
    printf("====================================================\n");
}

int main(void) {
    Inventory inv;
    int choice;
    int running = 1;
    Date today;
    int i;
    int expiredCount = 0;

    initInventory(&inv);
    printf("Welcome to the Hospital Pharmacy Stock System\n");
    loadInventory(&inv);

    today = getToday();
    for (i = 0; i < inv.count; i++) {
        if (isExpired(&inv.items[i], today)) {
            expiredCount++;
        }
    }
    if (expiredCount > 0) {
        printf("\n*** ALERT: %d expired drug(s) in stock. Choose option 8 for details. ***\n", expiredCount);
    }

    while (running) {
        showMenu();
        choice = readInt("Enter your choice: ", 0, 14);
        switch (choice) {
            case 1:
                addDrug(&inv);
                break;
            case 2:
                displayAll(&inv);
                break;
            case 3:
                searchById(&inv);
                break;
            case 4:
                searchByName(&inv);
                break;
            case 5:
                restockDrug(&inv);
                break;
            case 6:
                sellDrug(&inv);
                break;
            case 7:
                updatePrice(&inv);
                break;
            case 8:
                expiryAlerts(&inv);
                break;
            case 9:
                lowStockReport(&inv);
                break;
            case 10:
                sortInventory(&inv);
                break;
            case 11:
                inventorySummary(&inv);
                break;
            case 12:
                removeDrug(&inv);
                break;
            case 13:
                removeExpired(&inv);
                break;
            case 14:
                saveAndReport(&inv);
                break;
            case 0:
                saveAndReport(&inv);
                running = 0;
                break;
            default:
                break;
        }
    }

    freeInventory(&inv);
    printf("Exiting system. Stay safe!\n");
    return 0;
} 
