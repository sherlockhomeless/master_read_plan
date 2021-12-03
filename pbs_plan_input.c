#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>
#include <linux/sched.h>

//fixme: hardcode is bad
#include "config.h"
#include "pbs_entities.h"


   // Required for the copy to user function
#define  DEVICE_NAME "pbs_plan_in"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "device_class"        ///< The device class -- this is a character device driver

#define CHECK_PLAN 1

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("ml");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("base module");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  dev_class  = NULL; ///< The device-driver class struct pointer
static struct device* dev_device = NULL; ///< The device-driver device struct pointer


static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t plan_write(struct file *, const char *, size_t, loff_t *);

/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = plan_write,
   .release = dev_release,
};

static void fix_pointer_for_kernel_space(struct PBS_Plan*);

static char check_plan(struct PBS_Plan*);
static long length_plan(struct PBS_Plan* plan);
static void print_process(struct PBS_Process* pro);
struct PBS_Plan* plan_ptr;
int bytes_written;

extern struct PBS_Plan* get_pbs_plan(void);



/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init init_global_module(void){
    // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "[PBS_init_module] failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "[PBS_init_module]: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   dev_class = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(dev_class)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(dev_class);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "[PBS_init_module]: device class registered correctly\n");

   // Register the device driver
   dev_device = device_create(dev_class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(dev_device)){               // Clean up if there is an error
      class_destroy(dev_class);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(dev_device);
   }
   plan_ptr = get_pbs_plan();
   bytes_written = 0;
   printk(KERN_INFO "[PBS_init_module]0: read plan from address=%p\n", plan_ptr);
   printk(KERN_INFO "[PBS_init_module]0: Plan size in Kernel %zu\n", sizeof(struct PBS_Plan)); // Made it! device was initialized
   printk(KERN_INFO "[PBS_init_module]0: First Task length: %ld \n", plan_ptr->tasks[0].instructions_planned); // Made it! device was initialized
   printk(KERN_INFO "[PBS_init_module]0: device class created correctly\n"); // Made it! device was initialized

    return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit exit_global_module(void){
   device_destroy(dev_class, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(dev_class);                          // unregister the device class
   class_destroy(dev_class);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "[PBS_exit_module]: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_ALERT "OPEN: value num_processes: %#lx\n", plan_ptr->num_processes);
   return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
  //printk(KERN_ALERT "dev_read called, global_task_lateness = %ld", global_pbs_task.lateness);

  printk(KERN_INFO "[PBS_plan_copy_dev_read]: plan_ptr=%p\n", plan_ptr);
  printk(KERN_INFO "[PBS_plan_copy_dev_read]: tasks=%p, processes=%p\n", plan_ptr->tasks, plan_ptr->processes);
  printk(KERN_INFO "[PBS_plan_copy_dev_read]: num_processes=%ld, num_tasks=%ld\n", plan_ptr->num_processes, plan_ptr->num_tasks);
  printk(KERN_INFO "[PBS_plan_copy_dev_read]: buffer last process=%ld, length last task=%ld\n", plan_ptr->processes[2].buffer, plan_ptr->tasks[plan_ptr->num_tasks-1].instructions_planned);
  printk(KERN_INFO "[PBS_plan_copy_dev_read]: total plan length=%ld\n", length_plan(plan_ptr));
  return 0;
}

static ssize_t plan_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
    char plan_correct;
    short reset = 0;
    int err = 0;
    // https://www.kernel.org/doc/htmldocs/kernel-api/API---copy-from-user.html
    void* location_plan = plan_ptr;
    if (bytes_written > 0){
        printk(KERN_INFO "[PBS_plan_write] %d + %zu = %zu", bytes_written, len, bytes_written + len);
        location_plan += bytes_written;
        //FIXME: Very unflexible reset, reset should be set if rest_size - write_size > 0
        reset = 1;
    }
    err = copy_from_user(location_plan, buffer, len);
    if (err != 0 ){
        printk(KERN_ALERT "[PBS_plan_write] could not write to device");
    }
    bytes_written = len;
    // if reset == true => plan was fully written
    if (reset){
        bytes_written = 0;
        fix_pointer_for_kernel_space(plan_ptr);

        if (CHECK_PLAN){
            plan_correct = check_plan(plan_ptr);
            if (plan_correct){
                printk(KERN_INFO "[PBS_plan_write] Plan checked and no problems found");
            } else {
                printk(KERN_ERR "[PBS_plan_write] Plan checked and problems found");
            }
        }

    }
    return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
    printk(KERN_INFO "Device successfully closed\n");
    return 0;
}

static void fix_pointer_for_kernel_space(struct PBS_Plan* p){
    printk(KERN_INFO "[PBS_plan_write]0: fixing pointers, before: cur_task=%p, cur_process=%p\n", p->cur_task, p->cur_process);
    p->cur_task = p->tasks;
    p->cur_process = &p->processes[p->cur_task->process_id];
    p->finished_tasks = p->tasks;
    printk(KERN_INFO "[PBS_plan_write]0: fixing pointers, after: cur_task=%p, cur_process=%p\n", p->cur_task, p->cur_process);
    printk(KERN_INFO "[PBS_plan_write]0: plan_ptr=%p, processes=%p, tasks=%p\n", p, p->processes, p->tasks);
    printk(KERN_INFO "[PBS_plan_write]0: 1st_process=%p, 2nd_process=%p\n", &p->processes[0], &p->processes[1]);
    printk(KERN_INFO "[PBS_plan_write]0: 1st_task=%p, last_task=%p\n", &p->tasks[0], &p->tasks[p->num_tasks-1]);
    printk(KERN_INFO "[PBS_plan_write]0: 1st_process: id=%ld, num_tasks_remaining=%ld, instructions_retired=%ld, buffer=%ld\n", p->processes[0].process_id, p->processes[0].num_tasks_remaining, p->processes[0].instructions_retired, p->processes[0].buffer);
    printk(KERN_INFO "[PBS_plan_write]0: 1st_task: id=%ld, instructions_planned=%ld, instructions_retired=%ld, lateness=%ld\n", p->tasks[0].task_id, p->tasks[0].instructions_planned, p->tasks[0].instructions_retired_task, p->tasks[0].lateness);

}

static char check_plan(struct PBS_Plan* p){
    int i;
    struct PBS_Process* p_ptr;
    struct PBS_Task* t_ptr;

    if (p->num_processes <= 0 || p->num_tasks <= 0){
        printk(KERN_ERR "[PBS_check_plan] num_processes/num_tasks false\n");
        return 0;
    }

    if( p->index_cur_task != 0 || p->lateness != 0 || p->instructions_retired != 0 || p->tick_counter != 0 || p->tasks_finished != 0 || p->stress != 0){
        printk(KERN_ERR "[PBS_check_plan] Plan tracking variables falsely initialized\n");
        return 0;
    }

    // check processes
    p_ptr = p->processes;
    for(i = 0; i < p->num_processes; i++){
        print_process(p_ptr);
        if (p_ptr->process_id < 0 || p_ptr->num_tasks_remaining <= 0 || p_ptr->buffer <= 0 || p_ptr->lateness != 0 || p_ptr->length_plan <= 0 || p_ptr->instructions_retired != 0 ){
            printk(KERN_ERR "[PBS_check_plan] Process tracking variables falsely initialized\n");
            return 0;
        }

        p_ptr++;
    }

    // check tasks
    t_ptr = p->tasks;
    while(t_ptr->task_id != -2){
        if (t_ptr->task_id < -2 || t_ptr->process_id < -2){
            printk(KERN_ERR "[PBS_check_plan] Task-id falsely initialized\n");
            return 0;
        }
        if (t_ptr-> instructions_planned <= 0 || t_ptr->instructions_real <= 0 || t_ptr->instructions_retired_slot != 0 || t_ptr->instructions_retired_task != 0 || t_ptr->lateness != 0 ){

            printk(KERN_ERR "[PBS_check_plan] Task tracking falsely initialized\n");
            return 0;
        }

        t_ptr++;
    }
    return 1;
}

// summs up the length of the plan for verification of correct plan copying
static long length_plan(struct PBS_Plan* plan){
    int i;
    long plan_length = 0;

    for (i = 0; i < MAX_NUMBER_TASKS_IN_PLAN; i++){
        plan_length += plan->tasks[i].instructions_planned;
    }

    return plan_length;
}


static void print_process(struct PBS_Process* pro){
    printk(KERN_INFO "id=%ld, num_tasks=%ld, buffer=%ld, lateness=%ld, length=%ld, instructions_retired=%ld\n",
                pro->process_id, pro->num_tasks_remaining, pro->buffer, pro->lateness, pro->length_plan, pro->instructions_retired);
}
module_init(init_global_module);
module_exit(exit_global_module);
