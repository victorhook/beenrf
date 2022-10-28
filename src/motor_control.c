#include "motor_control.h"
#include "core.h"
#include "state.h"


//#define PCA_DIFF 4083  // PWM Frequency 500 HZ, Period: 2ms
#define PCA_DIFF 4083
#define PCA_MAX  0xffff
#define PCA_MIN  (PCA_MAX - PCA_DIFF)

// Helper defines
#define set_pwm_m1(value) set_sfr_16(PCA0CPL0, PCA0CPH0, PCA_MIN+value)
#define set_pwm_m2(value) set_sfr_16(PCA0CPL1, PCA0CPH1, PCA_MIN+value)

#define MOTOR_NOT_BEING_PWM_VALUE 1

#define XBAR_MOTOR_LEFT  P1SKIP
#define XBAR_MOTOR_RIGHT P0SKIP

#define XBAR_MOTOR_LEFT_1 (P1SKIP_B0__SKIPPED | P1SKIP_B1__NOT_SKIPPED | P1SKIP_B2__SKIPPED | P1SKIP_B3__SKIPPED | P1SKIP_B4__SKIPPED | P1SKIP_B5__SKIPPED | P1SKIP_B6__SKIPPED | P1SKIP_B7__SKIPPED)
#define XBAR_MOTOR_LEFT_2 (P1SKIP_B0__NOT_SKIPPED | P1SKIP_B1__SKIPPED | P1SKIP_B2__SKIPPED | P1SKIP_B3__SKIPPED | P1SKIP_B4__SKIPPED | P1SKIP_B5__SKIPPED | P1SKIP_B6__SKIPPED | P1SKIP_B7__SKIPPED)

#define XBAR_MOTOR_RIGHT_1 (P0SKIP_B0__SKIPPED | P0SKIP_B1__SKIPPED | P0SKIP_B2__NOT_SKIPPED | P0SKIP_B3__SKIPPED | P0SKIP_B4__SKIPPED | P0SKIP_B5__SKIPPED | P0SKIP_B6__SKIPPED | P0SKIP_B7__SKIPPED)
#define XBAR_MOTOR_RIGHT_2 (P0SKIP_B0__SKIPPED | P0SKIP_B1__NOT_SKIPPED | P0SKIP_B2__SKIPPED | P0SKIP_B3__SKIPPED | P0SKIP_B4__SKIPPED | P0SKIP_B5__SKIPPED | P0SKIP_B6__SKIPPED | P0SKIP_B7__SKIPPED)

typedef enum {
  STEER_DIRECTION_RIGHT,
  STEER_DIRECTION_LEFT,
  STEER_DIRECTION_STRAIGHT
} steer_direction_t;

typedef enum {
  MOTOR_DIRECTION_FORWARD,
  MOTOR_DIRECTION_BACKWARD,
  MOTOR_DIRECTION_IDLE
} motor_direction_t;


// --- Helper functions --- //

#define pwm_disable() (PCA0CN0 = PCA0CN0_CR__STOP)

#define pwm_enable()  (PCA0CN0 = PCA0CN0_CR__RUN)

#define motors_off() \
  pwm_disable(); \
  P0SKIP = 0xff; \
  P1SKIP = 0xff; \
  PIN_MOTOR_RIGHT_2 = 0; \
  PIN_MOTOR_RIGHT_1 = 0; \
  PIN_MOTOR_LEFT_2  = 0; \
  PIN_MOTOR_LEFT_1  = 0;

static void route_pwm(const motor_direction_t motor_right, const motor_direction_t motor_left);

static void set_steer_direction(const steer_direction_t steer_direction, const motor_direction_t motor_direction) ;


status_t motor_control_init() {
    // PCA Initialization (for PWM)
    PCA0MD = PCA0MD_CPS__SYSCLK_DIV_12 |    // Set clock source
             PCA0MD_ECF__OVF_INT_ENABLED;   // Enabled interrupt on Timer/Counter Overflow
    // PCA 0
    PCA0CPM0 = PCA0CPM0_PWM16__16_BIT |     // 16-bit PWM
               PCA0CPM0_PWM__ENABLED;//  |     // Enable PWM

    // PCA 1
    PCA0CPM1 = PCA0CPM1_PWM16__16_BIT |     // 16-bit PWM
               PCA0CPM1_PWM__ENABLED;// |      // Enable PWM

    PCA0POL = PCA0POL_CEX0POL__INVERT | PCA0POL_CEX1POL__INVERT;

    PCA0CN0 = PCA0CN0_CR__RUN;              // Start PCA

    XBR1 |= XBR1_PCA0ME__CEX0_CEX1;

    // Enable interrupt
    EIE1 |= EIE1_EPCA0__ENABLED;

    // Initial value
    set_steer_direction(STEER_DIRECTION_STRAIGHT, MOTOR_DIRECTION_FORWARD);
    set_pwm_m1(0);
    set_pwm_m2(0);

    return STATUS_OK;
}


// BACK  IDLE   FORWARD
// 0     2041   4082
#define MOTOR_IDLE 2041

void motor_control_update() {
  //   XIN1    XIN2   Function
  //   PWM     0      Forward PWM, fast decay
  //   1       PWM    Forward PWM, slow decay
  //   0       PWM    Reverse PWM, fast decay
  //   PWM     1      Reverse PWM, slow decay


  // Update state
  state.current.steer = state.target.steer;
  state.current.thrust = state.target.thrust;

  steer_direction_t steer_direction;
  motor_direction_t motor_direction;
  uint16_t thrust;

  // Steering
  if (state.current.steer == MOTOR_IDLE)
  {
    steer_direction = STEER_DIRECTION_STRAIGHT;
  }
  else if (state.current.steer > MOTOR_IDLE)
  {
    steer_direction = STEER_DIRECTION_RIGHT;
  }
  else
  {
    steer_direction = STEER_DIRECTION_LEFT;
  }

  // Motor thrust
  if (state.current.thrust == MOTOR_IDLE)
  {
    motor_direction = MOTOR_DIRECTION_IDLE;
  }
  else if (state.current.thrust > MOTOR_IDLE)
  {
    motor_direction = MOTOR_DIRECTION_FORWARD;
    thrust = state.current.thrust - MOTOR_IDLE;
  }
  else
  {
    motor_direction = MOTOR_DIRECTION_BACKWARD;
    thrust = MOTOR_IDLE - state.current.thrust;
  }

  /*
  if (motor_direction == MOTOR_DIRECTION_IDLE)
  {
    PIN_MOTOR_LEFT_1 = 0;
    PIN_MOTOR_LEFT_2 = 0;
    PIN_MOTOR_RIGHT_1 = 0;
    PIN_MOTOR_RIGHT_2 = 0;
  }
  else if (motor_direction == MOTOR_DIRECTION_FORWARD)
  {
    PIN_MOTOR_LEFT_1 = 0;
    PIN_MOTOR_LEFT_2 = 1;
    PIN_MOTOR_RIGHT_1 = 0;
    PIN_MOTOR_RIGHT_2 = 1;
  }
  else if (motor_direction == MOTOR_DIRECTION_BACKWARD)
  {
    PIN_MOTOR_LEFT_1 = 1;
    PIN_MOTOR_LEFT_2 = 0;
    PIN_MOTOR_RIGHT_1 = 1;
    PIN_MOTOR_RIGHT_2 = 0;
  }
  */


  if (motor_direction == MOTOR_DIRECTION_IDLE && steer_direction == STEER_DIRECTION_STRAIGHT)
  {
    motors_off();
  }
  else
  {
    pwm_disable();
    set_steer_direction(steer_direction, motor_direction);

    if (motor_direction == MOTOR_DIRECTION_IDLE)
    {
      // We'll just spin on the spot, wheels going opposite direction.
      if (state.current.steer > MOTOR_IDLE)
      {
        thrust = state.current.steer - MOTOR_IDLE;
      }
      else
      {
        thrust = MOTOR_IDLE - state.current.steer;
      }
    }

    // Multiply with 2 to put thrust into correct PWM range (0-4082).
    thrust = (MOTOR_IDLE - thrust) << 1;

    set_pwm_m1(thrust);
    set_pwm_m2(thrust);

    pwm_enable();


  }

  delay(10);
}



static void route_pwm(const motor_direction_t motor_right, const motor_direction_t motor_left)
{
  // Right
  if (motor_right == MOTOR_DIRECTION_FORWARD)
  {
    XBAR_MOTOR_RIGHT = XBAR_MOTOR_RIGHT_1;
    PIN_MOTOR_RIGHT_2 = MOTOR_NOT_BEING_PWM_VALUE;
  }
  else if (motor_right == MOTOR_DIRECTION_BACKWARD)
  {
    XBAR_MOTOR_RIGHT = XBAR_MOTOR_RIGHT_2;
    PIN_MOTOR_RIGHT_1 = MOTOR_NOT_BEING_PWM_VALUE;
  }

  // Left
  if (motor_left == MOTOR_DIRECTION_FORWARD)
  {
    XBAR_MOTOR_LEFT = XBAR_MOTOR_LEFT_1;
    PIN_MOTOR_LEFT_2 = MOTOR_NOT_BEING_PWM_VALUE;
  }
  else if (motor_left == MOTOR_DIRECTION_BACKWARD)
  {
    XBAR_MOTOR_LEFT = XBAR_MOTOR_LEFT_2;
    PIN_MOTOR_LEFT_1 = MOTOR_NOT_BEING_PWM_VALUE;
  }

  //PIN_MOTOR_LEFT_1 = MOTOR_NOT_BEING_PWM_VALUE;
  //PIN_MOTOR_LEFT_2 = MOTOR_NOT_BEING_PWM_VALUE;
  //PIN_MOTOR_RIGHT_1 = MOTOR_NOT_BEING_PWM_VALUE;
  //PIN_MOTOR_RIGHT_2 = MOTOR_NOT_BEING_PWM_VALUE;
}

static void set_steer_direction(const steer_direction_t steer_direction, const motor_direction_t motor_direction) {
  switch (motor_direction)
  {
    case MOTOR_DIRECTION_FORWARD:
      route_pwm(MOTOR_DIRECTION_FORWARD, MOTOR_DIRECTION_FORWARD);
      break;
    case MOTOR_DIRECTION_BACKWARD:
      route_pwm(MOTOR_DIRECTION_BACKWARD, MOTOR_DIRECTION_BACKWARD);
      break;
    case MOTOR_DIRECTION_IDLE:
      if (steer_direction == STEER_DIRECTION_RIGHT)
      {
        route_pwm(MOTOR_DIRECTION_BACKWARD, MOTOR_DIRECTION_FORWARD);
      }
      else if (steer_direction == STEER_DIRECTION_LEFT)
      {
        route_pwm(MOTOR_DIRECTION_FORWARD, MOTOR_DIRECTION_BACKWARD);
      }
      break;
  }

}

// --- PWM Interrupt --- /
SI_INTERRUPT(PCA0_ISR, PCA0_IRQn) {

// TODO: CHECK FLAGS!
//if (PCA0CN0 & PCA0CN0_CCF0__BMASK) {
//  set_sfr_16(PCA0L, PCA0H, PCA_MIN);
//  PCA0CN0 &= ~PCA0CN0_CCF0__BMASK;  // Clear flag
//} else if (PCA0CN0 & PCA0CN0_CCF1__BMASK) {
//  set_sfr_16(PCA0L, PCA0H, PCA_MIN);
//  PCA0CN0 &= ~PCA0CN0_CCF1__BMASK;  // Clear flag
//}

  // Stop PCA AND clear the Overflow flags.
  // Clearing the interrupt flags is important, because there are 4 different
  // flags that causes this ISR to trigger: ECF for CF, ECOV for COVF, and ECCFn for each CCFn
  PCA0CN0 = PCA0CN0_CR__STOP;
  set_sfr_16(PCA0L, PCA0H, PCA_MIN);    // Set current value of counter.
  PCA0CN0 = PCA0CN0_CR__RUN;
}
